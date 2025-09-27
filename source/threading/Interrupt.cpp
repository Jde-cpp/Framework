#include "Interrupt.h"
#include <jde/framework/thread/thread.h>
#include "InterruptibleThread.h"
#define let const auto
#ifdef Unused
namespace Jde::Threading{
	constexpr ELogTags _tags = ELogTags::Threads;
	Interrupt::Interrupt( sv threadName, Duration duration, bool paused ):
		_threadName{ threadName },
		_paused{ paused },
		_refreshRate{ duration }
	{
		Trace( _tags, "{0}::{0}(paused:  {1})", _threadName, paused );
		Start();
	}
	Interrupt::~Interrupt()
	{
		Stop();
	}
	void Interrupt::Start2()ι
	{
		_pThread = make_unique<Threading::InterruptibleThread>( _threadName, [&](){Worker();} );
	}
	void Interrupt::Start()ι
	{
		Trace( _tags, "{}::Start(paused:  {})", _threadName, (bool)_paused );
		std::call_once( _singleThread, &Interrupt::Start2, this );
	}
	void Interrupt::Wake()ι
	{
		Trace( _tags, "{}::Wake()", _threadName );
		_cvWait.notify_one();
	}
	void Interrupt::Pause()ι
	{
		Trace( _tags, "{}::Pause()", _threadName );
		_paused = true;
	}
	void Interrupt::UnPause()ι
	{
		Trace( _tags, "{}::UnPause()", _threadName );
		if( _paused )
		{
			_paused = false;
			_cvWait.notify_one();
		}
	}

	void Interrupt::Stop()ι
	{
		Trace( _tags, "{}::Stop()", _threadName );
		_pThread->Interrupt();
		Wake();
		_pThread->Join();
	}
	using namespace std::chrono_literals;
	void Interrupt::Worker()
	{
		Trace( _tags, "{}::Worker(paused:  {})", _threadName, (bool)_paused );
		SetThreadDscrptn( _threadName );
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
		Trace( _tags, "{}::Worker() - exiting", _threadName );
	}
}
#endif