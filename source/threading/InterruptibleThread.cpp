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
		if( GetDefaultLogger() )
			DBG( "~InterruptibleThread({})"sv, Name );
		if( ShouldJoin )
			Join();
	}
	void InterruptibleThread::Interrupt()noexcept
	{
		if( _pFlag && !_pFlag->IsSet() )
		{
			DBG( "{} - Interrupt _pFlag={}"sv, Name, _pFlag!=nullptr );
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
		DBG("{} - Shutdown"sv, Name);
		Interrupt();
	};
}