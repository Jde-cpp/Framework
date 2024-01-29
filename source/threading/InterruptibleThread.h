#pragma once
//adapted from https://livebook.manning.com/#!/book/c-plus-plus-concurrency-in-action-second-edition/chapter-9/v-7/1
#include <future>
#include <jde/Exception.h>
#include "Thread.h"
#include <jde/App.h>

namespace Jde::Threading{  //TODO Reflection remove Threading from public items. move to jthread
#define ω Γ static auto
	void InterruptionPoint()ε;
	struct Γ InterruptFlag
	{
		InterruptFlag()ι;
		void Set()ι;
		template<typename Lockable>
		void Wait( std::condition_variable_any& cv, Lockable& lk )ι;
		bool IsSet()Ι;
		bool IsDone()Ι{return _done;} void SetIsDone(){_done=true;}
	private:
		bool _done{false};
		std::atomic_flag _flag;
		std::condition_variable* _pThreadCondition{nullptr};
		std::condition_variable_any* _pThreadConditionAny{nullptr};
		std::mutex _setClearMutex;
	};
	extern thread_local InterruptFlag ThreadInterruptFlag;
	Γ InterruptFlag& GetThreadInterruptFlag()ι;

	struct InterruptibleThread : public IShutdown
	{
		template<typename FunctionType>
		InterruptibleThread( string name, FunctionType f )ι;
		virtual Γ ~InterruptibleThread();
		Γ void Interrupt()ι;
		Γ void Join();
		bool IsDone()Ι{ return _pFlag && _pFlag->IsDone(); }
		const string Name;
		Γ void Shutdown()ι override;
		void Detach()ι{ _internalThread.detach(); ShouldJoin = false; }//destructor on same thread.
	private:
		std::thread _internalThread;
		InterruptFlag* _pFlag{nullptr};
		bool ShouldJoin{true};
	};
#define _logTag LogTag()
	template<typename FunctionType>
	InterruptibleThread::InterruptibleThread( string name_, FunctionType f )ι:
		Name{ name_}
	{
		std::promise<InterruptFlag*> promise;
		_internalThread = std::thread( [f,&promise, name=move(name_)]
		{
			TRACE( "~InterruptibleThread::f({})", name );
			promise.set_value( &GetThreadInterruptFlag() );
			SetThreadDscrptn( name );
			f();
			TRACE( "~InterruptibleThread::f({})", name );
			GetThreadInterruptFlag().SetIsDone();
		});
		_pFlag = promise.get_future().get();
	}

	template<typename Lockable>
	void InterruptFlag::Wait( std::condition_variable_any& cv, Lockable& lk )ι
	{
		struct CustomLock
		{
			CustomLock( InterruptFlag* pThis, std::condition_variable_any& cond, Lockable& lk ):
				_pThis( pThis ),
				_lk( lk )
			{
				_pThis->_setClearMutex.lock();
				_pThis->_pThreadConditionAny=&cond;
			}
			void unlock()
			{
				_lk.unlock();
				_pThis->_setClearMutex.unlock();
			}
			void lock()
			{
				std::lock( _pThis->_setClearMutex, _lk );
			}
			~CustomLock()
			{
				_pThis->_pThreadConditionAny = 0;
				_pThis->_setClearMutex.unlock();
			}
			InterruptFlag* _pThis;
			Lockable& _lk;
		};
		CustomLock cl( this, cv, lk );
		InterruptionPoint();
		cv.wait( cl );
		InterruptionPoint();
	}
	struct ThreadInterrupted : public Exception{ ThreadInterrupted()ι:Exception{"interupted", ELogLevel::Trace}{} };
}
#undef _logTag 
#undef ω