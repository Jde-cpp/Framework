#include "Coroutine.h"
#include "../threading/InterruptibleThread.h"
#define var const auto

using namespace std::chrono;
namespace Jde::Coroutine
{
	std::shared_mutex CoroutinePool::_mtx;
	static var& _logLevel{ Logging::TagLevel("threads") };

	sp<CoroutinePool> CoroutinePool::_pInstance;
	std::atomic<uint> INDEX=0;
	ResumeThread::ResumeThread( str name, Duration idleLimit, CoroutineParam&& param )noexcept:
		IdleLimit{idleLimit},
		ThreadParam{ name, Threading::BumpThreadHandle() },
		_param{ move(param) },
		_thread{ [this]( stop_token stoken )
		{
			Threading::SetThreadDscrptn( format("CoroutineThread - {}", ThreadParam.AppHandle) );
			var index = INDEX++;
			auto timeout = Clock::now()+IdleLimit;
			while( !stoken.stop_requested() )
			{
				{
					unique_lock l{ _paramMutex };
					if( !_param )
					{
						var now = Clock::now();
						if( timeout>now )
							std::this_thread::yield();
						else
						{
							LOG( "({})CoroutineThread Stopping", index );
							_thread.request_stop();
						}
						continue;
					}
				}
				LOG( "({}:{})CoroutineThread call resume", index, _param->CoHandle.address() );
				_param->CoHandle.resume();
				LOG( "({})CoroutineThread finish resume", index );
				SetThreadInfo( ThreadParam );
				timeout = Clock::now()+IdleLimit;
				LOG( "({})CoroutineThread timeout={}", index, ToIsoString(timeout) );
				unique_lock l{ _paramMutex };
				_param = {};
			}
			LOG( "({})CoroutineThread::Done", index );
		}
	}
	{}
	ResumeThread::~ResumeThread()
	{
		if( !IApplication::ShuttingDown() )
			LOG( "({})ResumeThread::~ResumeThread", std::this_thread::get_id() );
		if( _thread.joinable() )
		{
			_thread.request_stop();
			_thread.join();
		}
	}
	optional<CoroutineParam> ResumeThread::Resume( CoroutineParam&& param )noexcept
	{
		ASSERT( param.CoHandle.address() );
		LOG( "({})ResumeThread::Resume", param.CoHandle.address() );
		unique_lock l{ _paramMutex };
		bool haveParam = _param.has_value();
		bool done = Done();
		auto result = !haveParam && !done ? optional<CoroutineParam>{} : optional<CoroutineParam>{ move(param) };
		if( !result )
			_param = move( param );
		return result;
	}

	Settings::Item<uint16> CoroutinePool::MaxThreadCount{ "coroutinePool/maxThreadCount", 100 };
	Settings::Item<Duration> CoroutinePool::WakeDuration{ "coroutinePool/wakeDuration", 5s };
	Settings::Item<Duration> CoroutinePool::ThreadDuration{ "coroutinePool/threadDuration", 1s };
	Settings::Item<Duration> CoroutinePool::PoolIdleThreshold{ "coroutinePool/poolIdleThreshold", 1s };


	void CoroutinePool::Shutdown()noexcept
	{
		if( _pThread )
		{
			_pThread->Interrupt();
			_pThread->Join();
		}
	}

	void CoroutinePool::Resume( coroutine_handle<>&& h/*, Threading::ThreadParam&& param*/ )noexcept
	{
		if( IApplication::ShuttingDown() )
		{
			DBG( "Can't resume, shutting down."sv );
			return;
		}
		{
			unique_lock l{ _mtx };
			if( !_pInstance )
			{
				LOG( "MaxThreadCount={}, WakeDuration={} ThreadDuration={}, PoolIdleThreshold={}", (uint16)MaxThreadCount, Chrono::ToString(WakeDuration), Chrono::ToString(ThreadDuration), Chrono::ToString(PoolIdleThreshold) );
				_pInstance = make_shared<CoroutinePool>();
				IApplication::AddShutdown( _pInstance );
			}
		}
		_pInstance->InnerResume( CoroutineParam{move(h)} );
	}

	void CoroutinePool::InnerResume( CoroutineParam&& param )noexcept
	{
		unique_lock l{ _mtx };
		if( _pQueue!=nullptr )
			_pQueue->Push( move(param) );
		else
		{
			l.unlock();
			StartThread( move(param) );
		}
	}
	optional<CoroutineParam> CoroutinePool::StartThread( CoroutineParam&& param )noexcept
	{
		unique_lock _{ _mtx };
		optional<CoroutineParam> pResult{ move(param) };
		for( auto p = _threads.begin(); p!=_threads.end(); p = p->Done() ? _threads.erase(p) : next(p) )
		{
			if( pResult )
				pResult = p->Resume( move(*pResult) );
		}
		if( pResult )
		{
			if( _threads.size()<MaxThreadCount )
			{
				_threads.emplace_back( format("CoroutinePool[{}]", _threads.size()), PoolIdleThreshold, move(*pResult) );
				pResult = {};
			}
			else if( !_pQueue )
			{
				_pQueue = mu<QueueMove<CoroutineParam>>();
				_pQueue->Push( move(param) );
				if( _pThread )
					_pThread->Join();
				_pThread = make_unique<Threading::InterruptibleThread>( string{Name}, [this](){Run();} );
			}
		}
		return pResult;
	}

	void CoroutinePool::Run()noexcept
	{
		Threading::SetThreadInfo( Threading::ThreadParam{ string{Name}, (uint)Threading::EThread::CoroutinePool} );
		LOG( "{} - Starting", Name );
		TimePoint quitTime = Clock::now()+(Duration)ThreadDuration;
		while( !Threading::GetThreadInterruptFlag().IsSet() ||  !_pQueue->empty() )
		{
			for( auto h = _pQueue->TryPop( WakeDuration ); h; )
			{
				h = StartThread( move(h.value()) );
				quitTime = Clock::now()+(Duration)ThreadDuration;
			}
			if( quitTime<Clock::now() )
			{
				unique_lock l{ _mtx };
				_pQueue = nullptr;
				break;
			}
		}
		LOG( "{} - Ending", Name );
	}
}