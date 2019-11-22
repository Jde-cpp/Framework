#include "stdafx.h"
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
			DBG( "~InterruptibleThread({})", Name );
		Join();
	}
	void InterruptibleThread::Interrupt()noexcept
	{
		DBG( "{} - Interrupt _pFlag={}", Name, _pFlag!=nullptr );
		if( _pFlag )
			_pFlag->Set();
	}
	void InterruptibleThread::Join()
	{
		if( _internalThread.joinable() )//ProtoClient::Run
			_internalThread.join();
	}
	void InterruptibleThread::Shutdown()noexcept
	{ 
		DBG("{} - Shutdown", Name); 
		Interrupt(); 
	};

	ThreadInterrupted::ThreadInterrupted():
		Exception( ELogLevel::Trace, "interupted" )
	{}

}