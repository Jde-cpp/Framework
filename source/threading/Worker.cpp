#include "Worker.h"
#include "Thread.h"
#include "InterruptibleThread.h"
#include "../io/FileCo.h"
//#include <signal.h>
//#include "../../../Linux/source/LinuxDrive.h"

#define var const auto
#define _logTag LogTag()
namespace Jde::Threading{
	sp<IWorker> IWorker::_pInstance;
	std::atomic_flag IWorker::_mutex;
	IWorker::IWorker( sv name )ι:
		NameInstance{ name },
		ThreadCount{ Settings::Get<uint8>(Jde::format("workers/{}/threads", name)).value_or(0) }
	{}

	IWorker::~IWorker(){}//abstract

	α IWorker::Initialize()ι->void
	{
	}


	α IWorker::StartThread()ι->void
	{
		_pThread = mu<std::jthread>( [&]( stop_token st ){ Run( st );} );
	}

	α IWorker::Run( stop_token /*st*/ )ι->void
	{
	}

	α IWorker::Shutdown( bool /*terminate*/ )ι->void{
		AtomicGuard l{ _mutex };
		if( _pThread ){
			_pThread->request_stop();
			_pThread->join();
			_pThread = nullptr;
		}
	}

	α IPollWorker::WakeUp()ι->void{
		AtomicGuard l{ _mutex };
		++_calls;
		if( ThreadCount && !_pThread )
			StartThread();
		else if( !ThreadCount && _calls==1 )
			IApplication::AddActiveWorker( this );
	}

	α IPollWorker::Sleep()ι->void{
		AtomicGuard l{ _mutex };
		--_calls;
		_lastRequest = Clock::now();
		if( !_calls && !ThreadCount )
			IApplication::RemoveActiveWorker( this );
	}

	α IPollWorker::Run( stop_token st )ι->void
	{
		Threading::SetThreadDscrptn( NameInstance );
		sp<IWorker> pKeepAlive;
		var keepAlive = Settings::Get<Duration>( "WorkerkeepAlive" ).value_or( 5s );
		TRACE( "({})Starting Thread", NameInstance );
		_lastRequest = Clock::now();
		while( !st.stop_requested() )
		{
			if( var p = Poll(); p )//null=nothing left to process, false=stuff to process, but not ready, true=processed
				continue;
			TimePoint lastRequest = _lastRequest;
			if( !_calls && Clock::now()>lastRequest+keepAlive )
			{
				AtomicGuard l{ _mutex };
				_pThread->request_stop();
				_pThread->detach();
				_pThread = nullptr;
			}
			else
				std::this_thread::yield();
		}
		TRACE( "({})Ending Thread", NameInstance );
	}
}