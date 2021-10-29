#pragma once
//adapted from https://livebook.manning.com/#!/book/c-plus-plus-concurrency-in-action-second-edition/chapter-9/v-7/1
#include <future>
#include <jde/Exception.h>
#include "Thread.h"
#include <jde/App.h>

namespace Jde::Threading  //TODO Reflection remove Threading from public items.
{
	void InterruptionPoint()noexcept(false);
	struct Γ InterruptFlag
	{
		InterruptFlag()noexcept;
		void Set()noexcept;
		template<typename Lockable>
		void Wait( std::condition_variable_any& cv, Lockable& lk );
		bool IsSet()const noexcept;
		bool IsDone()const noexcept{return _done;} void SetIsDone(){_done=true;}
	private:
		bool _done{false};
		std::atomic<bool> _flag{false};
		std::condition_variable* _pThreadCondition{nullptr};
		std::condition_variable_any* _pThreadConditionAny{nullptr};
		std::mutex _setClearMutex;
	};
	extern thread_local InterruptFlag ThreadInterruptFlag;
	Γ InterruptFlag& GetThreadInterruptFlag()noexcept;

	struct InterruptibleThread : public IShutdown
	{
		template<typename FunctionType>
		InterruptibleThread( sv name, FunctionType f )noexcept;
		virtual Γ ~InterruptibleThread();
		Γ void Interrupt()noexcept;
		Γ void Join();
		bool IsDone()const noexcept{ return _pFlag && _pFlag->IsDone(); }
		const string Name;
		Γ void Shutdown()noexcept override;
		void Detach()noexcept{ _internalThread.detach(); ShouldJoin = false; }//destructor on same thread.
	private:
		std::thread _internalThread;
		InterruptFlag* _pFlag{nullptr};
		bool ShouldJoin{true};
	};

	template<typename FunctionType>
	InterruptibleThread::InterruptibleThread( sv name, FunctionType f )noexcept:
		Name{name}
	{
		std::promise<InterruptFlag*> promise;
		string nameCopy{ Name };
		_internalThread = std::thread( [f,&promise, nameCopy]
		{
			promise.set_value( &GetThreadInterruptFlag() );
			SetThreadDscrptn( nameCopy );
			f();
			DBG( "~InterruptibleThread::f({})"sv, nameCopy );
			GetThreadInterruptFlag().SetIsDone();
		});
		_pFlag = promise.get_future().get();
	}

	template<typename Lockable>
	void InterruptFlag::Wait( std::condition_variable_any& cv, Lockable& lk )
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
	struct ThreadInterrupted : public Exception{ ThreadInterrupted()noexcept:Exception{"interupted", ELogLevel::Trace}{} };
}