#include "Interrupt.h"
#include "Thread.h"
#include "InterruptibleThread.h"
#define var const auto

namespace Jde::Threading
{
	constexpr std::chrono::seconds RefreshRate = std::chrono::seconds(10);
	
/*	Interrupt::Interrupt( string_view threadName, bool paused=false ):
		_threadName{ threadName },
*/
	Interrupt::Interrupt( string_view threadName, Duration duration, bool paused ):
		_threadName{ threadName },
		_paused{ paused },
		_refreshRate{ duration }
	{
		LOG( _logLevel, "{0}::{0}(paused:  {1})", _threadName, paused );
		Start();
	}
	Interrupt::~Interrupt()
	{
		Stop();
	}
	void Interrupt::Start2()noexcept
	{
		//_continue = true; 
		//_pThread = make_unique<std::thread>( [&](){Worker();} ); 
		_pThread = make_unique<Threading::InterruptibleThread>( _threadName, [&](){Worker();} );
	}
	void Interrupt::Start()noexcept
	{
		LOG( _logLevel, "{}::Start(paused:  {})", _threadName, (bool)_paused );
		std::call_once( _singleThread, &Interrupt::Start2, this );
	}
	void Interrupt::Wake()noexcept
	{
		LOG( _logLevel, "{}::Wake()", _threadName );
		_cvWait.notify_one();
	}
	void Interrupt::Pause()noexcept
	{
		LOG( _logLevel, "{}::Pause()", _threadName );
		_paused = true;
	}
	void Interrupt::UnPause()noexcept
	{
		LOG( _logLevel, "{}::UnPause()", _threadName );
		if( _paused )
		{
			_paused = false;
			_cvWait.notify_one();
		}
	}

	void Interrupt::Stop()noexcept
	{
		LOG( _logLevel, "{}::Stop()", _threadName );
		_pThread->Interrupt();
		//_continue = false;
		Wake();
		_pThread->Join();
	}
	using namespace std::chrono_literals;
	void Interrupt::Worker()
	{
		LOG( _logLevel, "{}::Worker(paused:  {})", _threadName, (bool)_paused );
		Threading::SetThreadDescription( _threadName );
		std::cv_status status = std::cv_status::timeout;
    	std::unique_lock<std::mutex> lk( _cvMutex );
    	while( !GetThreadInterruptFlag().IsSet() )
		{
			bool paused = _paused;
			if( !paused )
			{
				if( status==std::cv_status::timeout )
					OnTimeout();//Throws when gets called before constructor exits
				else
					OnAwake();
			}
			if( _refreshRate>0ns )
				status = _cvWait.wait_until( lk, std::chrono::steady_clock::now()+_refreshRate );
			else
			{
				_cvWait.wait( lk );
				status = std::cv_status::no_timeout;
			}
		}
		LOG( _logLevel, "{}::Worker() - exiting", _threadName );
	}
}