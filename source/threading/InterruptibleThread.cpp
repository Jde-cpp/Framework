#include "InterruptibleThread.h"

#ifdef Unused
#define let const auto
namespace Jde::Threading{
	constexpr ELogTags _tags = ELogTags::Threads;

	thread_local InterruptFlag ThreadInterruptFlag;
	InterruptFlag& GetThreadInterruptFlag()ι{return ThreadInterruptFlag;}
	void InterruptionPoint()ε
	{
		THROW_IFX( ThreadInterruptFlag.IsSet(), ThreadInterrupted() );
	}

	InterruptFlag::InterruptFlag()ι:
		_pThreadCondition(nullptr),
		_pThreadConditionAny(nullptr)
	{}

	bool InterruptFlag::IsSet()Ι
	{
		return _flag.test( std::memory_order_relaxed );
	}

	void InterruptFlag::Set()ι
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
		//LOG(  "~InterruptibleThread({})", Name );
	}
	void InterruptibleThread::Interrupt()ι
	{
		if( _pFlag && !_pFlag->IsSet() )
		{
			Debug( _tags, "{} - Interrupt _pFlag={}", Name, _pFlag!=nullptr );
			_pFlag->Set();
		}
	}
	void InterruptibleThread::Join(){
		if( _internalThread.joinable() )
			_internalThread.join();
	}
	void InterruptibleThread::Shutdown( bool /*terminate*/ )ι{
		Debug( _tags, "{} - Shutdown", Name );
		Interrupt();
	};
}
#endif