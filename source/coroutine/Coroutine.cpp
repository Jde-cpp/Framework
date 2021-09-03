#include "Coroutine.h"
#include "../threading/InterruptibleThread.h"
#ifdef _MSC_VER
	using std::stop_token;
#else
	using Jde::stop_token;
#endif
#define var const auto

using namespace std::chrono;
namespace Jde::Coroutine
{
	std::shared_mutex CoroutinePool::_mtx;
	ELogLevel CoroutinePool::_level{ ELogLevel::None };
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
							LOG( CoroutinePool::LogLevel(), "({})CoroutineThread Stopping", index );
							_thread.request_stop();
						}
						continue;
					}
				}
				LOG( CoroutinePool::LogLevel(), "({}:{})CoroutineThread call resume", index, _param->CoHandle.address() );
				_param->CoHandle.resume();
				LOG( CoroutinePool::LogLevel(), "({})CoroutineThread finish resume", index );
				SetThreadInfo( ThreadParam );
				timeout = Clock::now()+IdleLimit;
				LOG( CoroutinePool::LogLevel(), "({})CoroutineThread timeout={}", index, ToIsoString(timeout) );
				unique_lock l{ _paramMutex };
				_param = {};
			}
			LOG( CoroutinePool::LogLevel(), "({})CoroutineThread::Done", index );
		}
	}
	{}
	ResumeThread::~ResumeThread()
	{
		if( !IApplication::ShuttingDown() )
			LOG( CoroutinePool::LogLevel(), "({})ResumeThread::~ResumeThread", std::this_thread::get_id() );
		if( _thread.joinable() )
		{
			_thread.request_stop();
			_thread.join();
		}
	}
	optional<CoroutineParam> ResumeThread::Resume( CoroutineParam&& param )noexcept
	{
		ASSERT( param.CoHandle.address() );
		LOG( CoroutinePool::LogLevel(), "({})ResumeThread::Resume", param.CoHandle.address() );
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
			DBG( "Can't resume, shutting down."sv );
			return;
		}
		{
			unique_lock l{ _mtx };
			if( !_pSettings )
			{
				auto addSettings = []()noexcept
				{
					INFO( "coroutinePool settings not found."sv );
					nlohmann::json j2 = { {"maxThreadCount", 100}, {"threadDuration", "PT5S"}, {"poolIdleThreshold", "PT60S"} };
					_pSettings = make_shared<Settings::Container>( j2 );
				};
				try
				{
					if( Settings::Global().Have("coroutinePool") )
						_pSettings = Settings::Global().SubContainer( "coroutinePool" );
					else
						addSettings();
				}
				catch( EnvironmentException& )
				{
					addSettings();
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
		LOG( CoroutinePool::LogLevel(), "{} - Starting", Name );
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
		LOG( CoroutinePool::LogLevel(), "{} - Ending", Name );
	}
}
