#include "Coroutine.h"
#include "../threading/InterruptibleThread.h"
//#include "../threading/Thread.h"
//#include <chrono>
#define var const auto

using namespace std::chrono;
namespace Jde::Coroutine
{
	typedef uint ClientHandle;
	std::atomic<ClientHandle> _handleIndex{0};
	std::atomic<ClientHandle> TaskHandle{0};
	std::atomic<ClientHandle> TaskPromiseHandle{0};

	std::shared_mutex CoroutinePool::_mtx;
	sp<CoroutinePool> CoroutinePool::_pInstance;
	std::atomic<uint> INDEX=0;
	ResumeThread::ResumeThread( const string& name, Duration idleLimit, CoroutineParam&& param )noexcept:
		IdleLimit{idleLimit},
		ThreadParam{ name, Threading::BumpThreadHandle() },
		_param{ move(param) },
		_thread{ [this]( stop_token stoken )
		{
			var index = INDEX++;
			//DBG( "({})ResumeThread::ResumeThread"sv, std::this_thread::get_id() );
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
							//DBG( "{}>{}"sv, ToIsoString(timeout), ToIsoString(now) );
							//DBG( "timeout={}"sv, ToIsoString(timeout) );
							//DBG( "diff={} vs {}"sv, duration_cast<milliseconds>(now-(timeout-IdleLimit)).count(), duration_cast<milliseconds>(IdleLimit).count() );
							DBG( "({})CoroutineThread Stopping"sv, index );
							_thread.request_stop();
						}
						continue;
					}
				}
				//SetThreadInfo( *_param ); let awaitable do this.
				DBG( "({})CoroutineThread call resume"sv, index );
				_param->CoHandle.resume();
				DBG( "({})CoroutineThread finish resume"sv, index );
				SetThreadInfo( ThreadParam );
				timeout = Clock::now()+IdleLimit;
				DBG( "({})CoroutineThread timeout={}"sv, index, ToIsoString(timeout) );
				unique_lock l{ _paramMutex };
				_param = {};
			}
			DBG( "({})CoroutineThread::Done"sv, index );
		}
	}
	{}
	ResumeThread::~ResumeThread()
	{
		DBG( "({})ResumeThread::~ResumeThread"sv, std::this_thread::get_id() );
		if( _thread.joinable() )
			_thread.join();
		//_thread.request_stop(); s/b auto
	}
	optional<CoroutineParam> ResumeThread::Resume( CoroutineParam&& param )noexcept
	{
		unique_lock l{ _paramMutex };
		bool haveParam = _param.has_value();
		bool done = Done();
		auto result = !haveParam && !done ? optional<CoroutineParam>{} : optional<CoroutineParam>{ move(param) };
		if( !result )
			_param = move( param );
		return result;
	}

	sp<Settings::Container> CoroutinePool::_pSettings;
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
			DBG0( "Can't resume, shutting down."sv );
			return;
		}
		{
			unique_lock l{ _mtx };
			if( !_pSettings )
			{
/*				try
				{
					_pSettings = Settings::Global().SubContainer("coroutinePool");
				}
				catch( EnvironmentException& )*/
				{
					INFO0( "coroutinePool settings not found."sv );
					nlohmann::json j2 = { {"maxThreadCount", 100}, {"threadDuration", "PT5S"}, {"poolIdleThreshold", "PT60S"} };
					_pSettings = make_shared<Settings::Container>( j2 );
				}


			}
			if( !_pInstance )
			{
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
		unique_lock l{ _mtx };
		optional<CoroutineParam> pResult = move(param);
		for( auto p = _threads.begin(); p!=_threads.end(); p = p->Done() ? _threads.erase(p) : next(p) )
		{
			if( pResult )
				pResult = p->Resume( move(*pResult) );
		}
		if( pResult )
		{
			var max = MaxThreadCount();
			if( _threads.size()<max )
			{
				_threads.emplace_back( format("CoroutinePool[{}]", _threads.size()), PoolIdleThreshold(), move(*pResult) );
				pResult = {};
			}
			else if( !_pQueue )
			{
				_pQueue = make_unique<QueueMove<CoroutineParam>>();
				_pQueue->Push( move(param) );
				if( _pThread )
					_pThread->Join();
				_pThread = make_unique<Threading::InterruptibleThread>( Name, [this](){Run();} );
			}
		}
		return pResult;
	}

	void CoroutinePool::Run()noexcept
	{
		Threading::SetThreadInfo( Threading::ThreadParam{ string{Name}, (uint)Threading::EThread::CoroutinePool} );
		DBG( "{} - Starting"sv, Name );
		TimePoint quitTime = Clock::now()+ThreadDuration();
		while( !Threading::GetThreadInterruptFlag().IsSet() ||  !_pQueue->empty() )
		{
			for( auto h = _pQueue->TryPop( WakeDuration() ); h; )
			{
				h = StartThread( move(h.value()) );
				quitTime = Clock::now()+ThreadDuration();
			}
			if( quitTime<Clock::now() )
			{
				unique_lock l{ _mtx };
				_pQueue = nullptr;
				break;
			}
		}
		DBG( "{} - Ending"sv, Name );
	}
}
