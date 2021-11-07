#include "InterruptibleThread.h"

#define var const auto
namespace Jde::Threading
{
	static var _logLevel{ Logging::TagLevel("threads") };
	thread_local InterruptFlag ThreadInterruptFlag;
	InterruptFlag& GetThreadInterruptFlag()noexcept{return ThreadInterruptFlag;}
	void InterruptionPoint()noexcept(false)
	{
		THROW_IFX( ThreadInterruptFlag.IsSet(), ThreadInterrupted() );
	}

	InterruptFlag::InterruptFlag()noexcept:
		_pThreadCondition(nullptr),
		_pThreadConditionAny(nullptr)
	{}

	bool InterruptFlag::IsSet()const noexcept
	{
		return _flag.test( std::memory_order_relaxed );
	}

	void InterruptFlag::Set()noexcept
	{
		_flag.test_and_set( std::memory_order_relaxed );
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
		LOG(  "~InterruptibleThread({})", Name );
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