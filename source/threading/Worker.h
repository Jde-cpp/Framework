#pragma once
#include <jde/TypeDefs.h>
#include <jde/App.h>
#include "./jthread.h"
#include "./Mutex.h"
#include "../collections/Queue.h"
#include "InterruptibleThread.h"

namespace Jde::Threading
{
	#define var const auto
#ifdef _MSC_VER
	using std::stop_token;
#else
	using Jde::stop_token;
#endif
	/*handle signals, configuration*/
	struct JDE_NATIVE_VISIBILITY IWorker /*final*/: IShutdown, std::enable_shared_from_this<IWorker>
	{
		IWorker( sv name )noexcept;//:_name{name}{ DBG("IWorker::IWorker({})"sv, _name); }
		virtual ~IWorker()=0;
		virtual void Initialize()noexcept;
		void Shutdown()noexcept override;
		bool HasThread()noexcept{ return _pThread!=nullptr; }
		virtual void Run( stop_token st )noexcept;
		sv Name;
		const uint8 ThreadCount;
	protected:
		void StartThread()noexcept;
		optional<Settings::Container> Settings(){ return Settings::TryGetSubcontainer<Settings::Container>( "workers", Name ); }
		up<jthread> _pThread;
		static std::atomic<bool> _mutex;
	};
	struct JDE_NATIVE_VISIBILITY IPollWorker /*abstract*/ : IWorker, IPollster
	{
		IPollWorker( sv name )noexcept:IWorker{ name }{}
		virtual optional<bool> Poll()noexcept=0;
		void WakeUp()noexcept override;
		void Sleep()noexcept override;
	protected:
		void Run( stop_token st )noexcept override;
	private:
		atomic<uint> _calls{0};
		atomic<TimePoint> _lastRequest{ Clock::now() };
	};

	template<class T>
	struct TWorker /*abstract*/: IPollWorker
	{
		TWorker( sv name )noexcept:IPollWorker{ name }{}
	protected:
		Ω Start()noexcept->sp<IWorker>;
	};

	#define TARG template<class TArg, class TDerived>
	TARG struct IQueueWorker /*abstract*/: TWorker<TDerived>
	{
		using base=TWorker<TDerived>; using Class=IQueueWorker<TArg,TDerived>;
		IQueueWorker( sv threadName ):base{ threadName }{}
		Ω Push( TArg&& x )noexcept->void;
		virtual void HandleRequest( TArg&& x )noexcept=0;
	protected:
		bool Poll()noexcept override;
	private:
		static sp<TDerived> Start()noexcept{ return dynamic_pointer_cast<TDerived>(base::Start()); }
		QueueMove<TArg> _queue;
	};


/*	ⓣ TWorker<T>::Start()noexcept->sp<IWorker>
	{
		if( IApplication::ShuttingDown() )
			return nullptr;
		Threading::AtomicGuard l{ _mutex };
		if( !_pInstance )
		{
			_pInstance = make_shared<T>();
			IApplication::AddShutdown( _pInstance );
			var pSettings = Settings::TryGetSubcontainer<Settings::Container>( "workers", _pInstance->Name );
			if( pSettings && pSettings->Get2<uint8>("threads") )
				_pInstance->StartThread();
			else
				throw "not implemented";
			//_pInstance->_pThread = make_unique<jthread>([&]( stop_token st ){_pInstance->Run( st );} );
		}
		return _pInstance;
	}
*/
	TARG α IQueueWorker<TArg,TDerived>::Push( TArg&& x )noexcept->void
	{
		if( auto p=Start(); p )
		{
			p->_queue.Push( move(x) );
			if( p->HasThread() )
				x.SetWorker( p );
			else
				IApplication::AddActiveWorker( p );
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
#undef var
}