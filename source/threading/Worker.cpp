﻿#include "Worker.h"
#include "Thread.h"
#include "InterruptibleThread.h"
#include "../io/FileCo.h"
//#include <signal.h>
//#include "../../../Linux/source/LinuxDrive.h"

#define var const auto
namespace Jde::Threading
{
	static const LogTag& _logLevel{ Logging::TagLevel("threads") };
	sp<IWorker> IWorker::_pInstance;
	std::atomic_flag IWorker::_mutex;
	IWorker::IWorker( sv name )noexcept:
		NameInstance{ name },
		ThreadCount{ Settings::TryGet<uint8>(format("workers/{}/threads", name)).value_or(0) }
	{}

	IWorker::~IWorker(){}//abstract

	α IWorker::Initialize()noexcept->void
	{
	}
	

	α IWorker::StartThread()noexcept->void
	{
		_pThread = make_unique<jthread>( [&]( stop_token st ){ Run( st );} );
	}

	α IWorker::Run( stop_token /*st*/ )noexcept->void
	{
	}

	α IWorker::Shutdown()noexcept->void
	{
		AtomicGuard l{ _mutex };
		if( _pThread )
		{
			_pThread->request_stop();
			_pThread->join();
			_pThread = nullptr;
		}
	}
	α IPollWorker::WakeUp()noexcept->void
	{
		AtomicGuard l{ _mutex };
		++_calls;
		if( ThreadCount && !_pThread )
			StartThread();
		else if( !ThreadCount && _calls==1 )
			IApplication::AddActiveWorker( this );
	}
	α IPollWorker::Sleep()noexcept->void
	{
		AtomicGuard l{ _mutex };
		--_calls;
		_lastRequest = Clock::now();
		if( !_calls && !ThreadCount )
			IApplication::RemoveActiveWorker( this );
	}

	α IPollWorker::Run( stop_token st )noexcept->void
	{
		Threading::SetThreadDscrptn( NameInstance );
		sp<IWorker> pKeepAlive;
		var keepAlive = Settings::TryGet<Duration>( "WorkerkeepAlive" ).value_or( 5s );
		LOG( "({})Starting Thread", NameInstance );
		while( !st.stop_requested() )
		{
			if( var p = Poll(); !p || *p )
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
		LOG( "({})Ending Thread", NameInstance );
	}
}