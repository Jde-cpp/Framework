#include "Coroutine.h"
#include "../threading/InterruptibleThread.h"
#include "../DateTime.h"

#define let const auto

using namespace std::chrono;
namespace Jde::Coroutine{

	std::shared_mutex CoroutinePool::_mtx;
	static ELogTags _tag{ ELogTags::Threads };

	sp<CoroutinePool> CoroutinePool::_pInstance;
	std::atomic<uint> INDEX=0;
	ResumeThread::ResumeThread( str name, Duration idleLimit, CoroutineParam&& param )ι:
		IdleLimit{idleLimit},
		ThreadParam{ name, Threading::BumpThreadHandle() },
		_param{ move(param) },
		_thread{ [this]( std::stop_token stoken ){
			Threading::SetThreadDscrptn( Jde::format("({:x})Co", ThreadParam.AppHandle) );
			let index = INDEX++;
			auto timeout = Clock::now()+IdleLimit;
			while( !stoken.stop_requested() ){
				{
					unique_lock l{ _paramMutex };
					if( !_param ){
						let now = Clock::now();
						if( timeout>now )
							std::this_thread::yield();
						else{
							Trace( _tag, "({})CoroutineThread Stopping", index );
							_thread.request_stop();
						}
						continue;
					}
				}
				Trace( _tag, "({}:{}:{})CoroutineThread call resume - on thread:{:x}", ThreadParam.AppHandle, index, _param->CoHandle.address(), Threading::GetThreadId() );
				_param->CoHandle.resume();
				Trace( _tag, "({})CoroutineThread finish resume", index );
				SetThreadInfo( ThreadParam );
				timeout = Clock::now()+IdleLimit;
				Trace( _tag, "({})CoroutineThread timeout={}", index, ToIsoString(timeout) );
				unique_lock l{ _paramMutex };
				_param = {};
			}
			Trace( _tag, "({})CoroutineThread::Done", index );
		}
	}
	{}
	ResumeThread::~ResumeThread(){
		//if( !Process::ShuttingDown() )
		Trace( _tag, "({:x}:{})ResumeThread::~ResumeThread", Threading::GetThreadId(), ThreadParam.AppHandle );
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
			Trace( _tag, "({})ResumeThread::CheckIfReady - Reusing", ThreadParam.AppHandle );
			_param = move( param );
		}
		return result;
	}

	uint16 CoroutinePool::MaxThreadCount{ Settings::FindNumber<uint16>("/coroutinePool/maxThreadCount").value_or(100) };
	Duration CoroutinePool::WakeDuration{ Settings::FindDuration("/coroutinePool/wakeDuration").value_or(5s) };
	Duration CoroutinePool::ThreadDuration{ Settings::FindDuration("/coroutinePool/threadDuration").value_or(1s) };
	Duration CoroutinePool::PoolIdleThreshold{ Settings::FindDuration("/coroutinePool/poolIdleThreshold").value_or(1s) };

	α CoroutinePool::Shutdown( bool /*terminate*/ )ι->void{
		if( _pThread ){
			_pThread->Interrupt();
			_pThread->Join();
		}
	}

	α CoroutinePool::Resume( coroutine_handle<> h )ι->void{
		// if( Process::ShuttingDown() ){  Is this needed?
		// 	DBG( "Can't resume, shutting down."sv );
		// 	return;
		// }
		{
			unique_lock l{ _mtx };
			if( !_pInstance ){
				Information( _tag, "MaxThreadCount={}, WakeDuration={} ThreadDuration={}, PoolIdleThreshold={}", (uint16)MaxThreadCount, Chrono::ToString<Duration>(WakeDuration), Chrono::ToString<Duration>(ThreadDuration), Chrono::ToString<Duration>(PoolIdleThreshold) );
				_pInstance = ms<CoroutinePool>();
				Process::AddShutdown( _pInstance );
			}
		}
		_pInstance->InnerResume( CoroutineParam{h} );
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
			if( _threads.size()<MaxThreadCount ){
				_threads.emplace_back( Jde::format("CoroutinePool[{}]", _threads.size()), PoolIdleThreshold, move(*pResult) );
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
		Trace( _tag, "{} - Starting", Name );
		TimePoint quitTime = Clock::now()+(Duration)ThreadDuration;
		while( !Threading::GetThreadInterruptFlag().IsSet() ||  !_pQueue->empty() ){
			for( auto h = _pQueue->TryPop( WakeDuration ); h; ){
				h = StartThread( move(h.value()) );
				quitTime = Clock::now()+(Duration)ThreadDuration;
			}
			if( quitTime<Clock::now() ){
				unique_lock l{ _mtx };
				_pQueue = nullptr;
				break;
			}
		}
		Trace( _tag, "{} - Ending", Name );
	}
}