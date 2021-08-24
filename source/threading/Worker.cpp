﻿#include "Worker.h"
#include "Thread.h"
#include "InterruptibleThread.h"
#include "../io/FileCo.h"
#include <signal.h>
#include "../../../Linux/source/LinuxDrive.h"

#define var const auto
namespace Jde::Threading
{
	//sp<IWorker> IWorker::_pInstance;
	std::atomic<bool> IWorker::_mutex;
	IWorker::IWorker( sv name )noexcept:
		Name{ name },
		ThreadCount{ (Settings() ? Settings()->Get2<uint8>( "threads" ) : std::nullopt).value_or(0) }
	{
	}
	void IWorker::Initialize()noexcept
	{
	}
	IWorker::~IWorker(){}//abstract

	void IWorker::StartThread()noexcept
	{
		_pThread = make_unique<jthread>( [&]( stop_token st ){ Run( st );} );
	}

	void IWorker::Run( stop_token /*st*/ )noexcept
	{
	}

	α IWorker::Shutdown()noexcept->void
	{
		Threading::AtomicGuard l{ _mutex };
		if( _pThread )
		{
			_pThread->request_stop();
			_pThread->join();
			_pThread = nullptr;
		}
	}
	void IPollWorker::WakeUp()noexcept
	{
		Threading::AtomicGuard l{ _mutex };
		++_calls;
		if( ThreadCount && !_pThread )
			StartThread();
		else if( !ThreadCount && _calls==1 )
			IApplication::AddActiveWorker( this );
	}
	void IPollWorker::Sleep()noexcept
	{
		Threading::AtomicGuard l{ _mutex };
		--_calls;
		_lastRequest = Clock::now();
		if( !_calls && !ThreadCount )
			IApplication::RemoveActiveWorker( this );
	}

	void IPollWorker::Run( stop_token st )noexcept
	{
		Threading::SetThreadDscrptn( Name );
		sp<IWorker> pKeepAlive;
		var keepAlive = Settings::Get<Duration>( "WorkerkeepAlive" ).value_or( 5s );
		DBG( "{} - Starting"sv, Name );
		while( !st.stop_requested() )
		{
			if( Poll() )
				continue;
			TimePoint lastRequest = _lastRequest;
			if( !_calls && Clock::now()>lastRequest+keepAlive )
			{
				Threading::AtomicGuard l{ _mutex };
				_pThread->request_stop();
				_pThread->detach();
				_pThread = nullptr;
			}
			else
				std::this_thread::yield();
		}
		DBG( "{} - Ending"sv, Name );
	}
}