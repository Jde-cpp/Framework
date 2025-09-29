#include "Coroutine.h"
#include "../threading/InterruptibleThread.h"
#include <jde/framework/chrono.h>
#include <jde/framework/settings.h>

#ifdef Unused
#define let const auto

using namespace std::chrono;
namespace Jde::Coroutine{

	std::shared_mutex CoroutinePool::_mtx;
	static ELogTags _tag{ ELogTags::Threads };

	up<CoroutinePool> _instance;
	std::atomic<uint> INDEX=0;
	CoroutinePool::CoroutinePool()ι:
		_maxThreadCount{ Settings::FindNumber<uint16>("/coroutinePool/maxThreadCount").value_or(100) },
		_wakeDuration{ Settings::FindDuration("/coroutinePool/wakeDuration").value_or(5s) },
		_threadDuration{ Settings::FindDuration("/coroutinePool/threadDuration").value_or(1s) },
		_poolIdleThreshold{ Settings::FindDuration("/coroutinePool/poolIdleThreshold").value_or(10ms) }{
		INFO( "MaxThreadCount={}, WakeDuration={} ThreadDuration={}, PoolIdleThreshold={}", _maxThreadCount, Chrono::ToString<>(_wakeDuration), Chrono::ToString<>(_threadDuration), Chrono::ToString<>(_poolIdleThreshold) );
		Process::AddShutdownFunction( []( bool terminating ){
			_instance->Shutdown( terminating );
			_instance = nullptr;
		});
	}
	ResumeThread::ResumeThread( str name, Duration idleLimit, CoroutineParam&& param )ι:
		IdleLimit{idleLimit},
		ThreadParam{ name, Threading::BumpThreadHandle() },
		_param{ move(param) },
		_thread{ [this]( std::stop_token stoken ){
			SetThreadDscrptn( Ƒ("({:x})Co", ThreadParam.AppHandle) );
			let index = INDEX++;
			auto timeout = Clock::now()+IdleLimit;
			while( !stoken.stop_requested() ){
				{
					unique_lock l{ _paramMutex };
					if( !_param ){
						if( timeout>Clock::now() )
							std::this_thread::yield();
						else{
							TRACE( "({})CoroutineThread Stopping", index );
							_thread.request_stop();
						}
						continue;
					}
				}
				TRACE( "({}:{}:{})CoroutineThread call resume - on thread:{:x}", ThreadParam.AppHandle, index, _param->CoHandle.address(), ThreadId() );
				_param->CoHandle.resume();
				TRACE( "({})CoroutineThread finish resume", index );
				SetThreadInfo( ThreadParam );
				timeout = Clock::now()+IdleLimit;
				TRACE( "({})CoroutineThread timeout={}", index, ToIsoString(timeout) );
				unique_lock l{ _paramMutex };
				_param = {};
			}
			TRACE( "({})CoroutineThread::Done", index );
		}
	}
	{}
	ResumeThread::~ResumeThread(){
		//if( !Process::ShuttingDown() )
		TRACE( "({:x}:{})ResumeThread::~ResumeThread", ThreadId(), ThreadParam.AppHandle );
		if( _thread.joinable() ){
			_thread.request_stop();
			_thread.join();
		}
	}
	//Check if can use this thread, ie its not done, and parameter is not set.
	//returns null if this can be used, if not, the parameter passed in.
	α ResumeThread::CheckIfReady( CoroutineParam&& param )ι->optional<CoroutineParam>{
		ASSERT( param.CoHandle.address() );
		unique_lock l{ _paramMutex };
		bool haveParam = _param.has_value();
		bool done = Done();
		auto result = !haveParam && !done ? optional<CoroutineParam>{} : optional<CoroutineParam>{ move(param) };
		if( !result ){
			TRACE( "({})ResumeThread::CheckIfReady - Reusing", ThreadParam.AppHandle );
			_param = move( param );
		}
		return result;
	}

	α CoroutinePool::Shutdown( bool /*terminate*/ )ι->void{
		if( _pThread ){
			_pThread->Interrupt();
			_pThread->Join();
		}
	}

	α CoroutinePool::Resume( coroutine_handle<> h )ι->void{
		{
			unique_lock l{ _mtx };
			if( !_instance )
				_instance = mu<CoroutinePool>();
		}
		_instance->InnerResume( CoroutineParam{h} );
	}

	α CoroutinePool::InnerResume( CoroutineParam&& param )ι->void{
		unique_lock l{ _mtx };
		if( _pQueue!=nullptr )
			_pQueue->Push( move(param) );
		else{
			l.unlock();
			StartThread( move(param) );
		}
	}
	α Erase( std::list<ResumeThread>& _threads, std::list<ResumeThread>::iterator p )ι->std::list<ResumeThread>::iterator{
		auto nxt = _threads.erase( p );
		return nxt;
	}
	α CoroutinePool::StartThread( CoroutineParam&& param )ι->optional<CoroutineParam>{
		unique_lock _{ _mtx };
		optional<CoroutineParam> pResult{ move(param) };
		for( auto p = _threads.begin(); p!=_threads.end(); p = p->CanDelete() ? Erase( _threads, p ) : next(p) ){
			if( pResult )
				pResult = p->CheckIfReady( move(*pResult) );
		}
		if( pResult ){
			if( _threads.size()<_maxThreadCount ){
				_threads.emplace_back( Ƒ("CoroutinePool[{}]", _threads.size()), _poolIdleThreshold, move(*pResult) );
				pResult = {};
			}
			else if( !_pQueue ){
				_pQueue = mu<QueueMove<CoroutineParam>>();
				_pQueue->Push( move(param) );
				if( _pThread )
					_pThread->Join();
				_pThread = make_unique<Threading::InterruptibleThread>( string{Name}, [this](){Run();} );
			}
		}
		return pResult;
	}

	α CoroutinePool::Run()ι->void{
		Threading::SetThreadInfo( Threading::ThreadParam{ string{Name}, (uint)Threading::EThread::CoroutinePool} );
		TRACE( "{} - Starting", Name );
		TimePoint quitTime = Clock::now()+(Duration)_threadDuration;
		while( !Threading::GetThreadInterruptFlag().IsSet() ||  !_pQueue->empty() ){
			for( auto h = _pQueue->TryPop( _wakeDuration ); h; ){
				h = StartThread( move(h.value()) );
				quitTime = Clock::now()+(Duration)_threadDuration;
			}
			if( quitTime<Clock::now() ){
				unique_lock l{ _mtx };
				_pQueue = nullptr;
				break;
			}
		}
		TRACE( "{} - Ending", Name );
	}
}
#endif