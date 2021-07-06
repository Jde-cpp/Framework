#pragma once
#include <jde/TypeDefs.h>
#include <jde/App.h>
#include "./jthread.h"
#include "./Mutex.h"
#include "../Collections/Queue.h"
#include "InterruptibleThread.h"

namespace Jde::Threading
{
#ifdef _MSC_VER
	using std::stop_token;
#else
	using Jde::stop_token;
#endif
	struct JDE_NATIVE_VISIBILITY IWorker /*abstract*/: IShutdown, std::enable_shared_from_this<IWorker>
	{
		IWorker( sv name )noexcept;//:_name{name}{ DBG("IWorker::IWorker({})"sv, _name); }
		virtual ~IWorker()=0;
		void Shutdown()noexcept override;
	protected:
		α Run( stop_token st )noexcept->void;
		virtual bool Poll()noexcept=0;

		static sp<IWorker> _pInstance;
		sv _name;
		up<jthread> _pThread;
		static std::atomic<bool> _mutex;
	private:
		TimePoint _lastRequest{ Clock::now() };
	};
	
	template<class T>
	struct TWorker /*abstract*/: IWorker
	{
		TWorker( sv name )noexcept:IWorker{ name }{}
	protected:
		Ω Start()noexcept->sp<IWorker>;
	};

	#define TARG template<class TArg, class TDerived>
	TARG struct IQueueWorker /*abstract*/: TWorker<TDerived>
	{
		using base=TWorker<TDerived>; using Class=IQueueWorker<TArg,TDerived>;
		Ω Push( TArg&& x )noexcept->void;
		virtual void HandleRequest( TArg&& x )noexcept=0;
	protected:
		bool Poll()noexcept override;
	private:
		static sp<TDerived> Start()noexcept{ return dynamic_pointer_cast<TDerived>(base::Start()); }
		QueueMove<TArg> _queue;
	};

	ⓣ TWorker<T>::Start()noexcept->sp<IWorker>
	{
		if( IApplication::ShuttingDown() )
			return nullptr;
		Threading::AtomicGuard l{ _mutex };
		if( !_pInstance )
		{
	//		_pInstance = make_shared<T>();
			IApplication::AddShutdown( _pInstance );
			//_pInstance->_pThread = make_unique<jthread>([&]( stop_token st ){_pInstance->Run( st );} );
		}
		return _pInstance;
	}

	TARG α IQueueWorker<TArg,TDerived>::Push( TArg&& x )noexcept->void 
	{
		if( auto p=Start(); p )
		{
			x.SetWorker( p );
			p->_queue.Push( move(x) );
		}
	}
	TARG α IQueueWorker<TArg,TDerived>::Poll()noexcept->bool
	{
		bool handled = false;
		if( auto p = _queue.Pop(); p )
		{
			HandleRequest( std::move(*p) );
			handled = true;
		}
		return handled;
	}
#undef TARG
}