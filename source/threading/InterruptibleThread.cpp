#include "InterruptibleThread.h"

namespace Jde::Threading
{
	thread_local InterruptFlag ThreadInterruptFlag;
	InterruptFlag& GetThreadInterruptFlag()noexcept{return ThreadInterruptFlag;}
	void InterruptionPoint()noexcept(false)
	{
		if( ThreadInterruptFlag.IsSet() )
			throw ThreadInterrupted();
	}

	InterruptFlag::InterruptFlag()noexcept:
		_pThreadCondition(nullptr),
		_pThreadConditionAny(nullptr)
	{}

	bool InterruptFlag::IsSet()const noexcept
	{
		return _flag.load( std::memory_order_relaxed );
	}

	void InterruptFlag::Set()noexcept
	{
		_flag.store( true, std::memory_order_relaxed );
		std::lock_guard<std::mutex> lk( _setClearMutex );
		if( _pThreadCondition )
			_pThreadCondition->notify_all();
		else if( _pThreadConditionAny )
			_pThreadConditionAny->notify_all();
	}
	InterruptibleThread::~InterruptibleThread()
	{
		if( ShouldJoin )
			Join();
		TAG( "threads", "~InterruptibleThread({})", Name );
	}
	void InterruptibleThread::Interrupt()noexcept
	{
		if( _pFlag && !_pFlag->IsSet() )
		{
			DBG( "{} - Interrupt _pFlag={}", Name, _pFlag!=nullptr );
			_pFlag->Set();
		}

	}
	void InterruptibleThread::Join()
	{
		if( _internalThread.joinable() )
			_internalThread.join();
	}
	void InterruptibleThread::Shutdown()noexcept
	{
		DBG( "{} - Shutdown", Name );
		Interrupt();
	};
}