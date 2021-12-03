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
	struct Γ IWorker : IShutdown, std::enable_shared_from_this<IWorker>
	{
		IWorker( sv name )noexcept;
		virtual ~IWorker()=0;
		β Initialize()noexcept->void;
		α Shutdown()noexcept->void override;
		α HasThread()noexcept->bool{ return _pThread!=nullptr; }
		β Run( stop_token st )noexcept->void;
		sv NameInstance;
		const uint8 ThreadCount;
		α StartThread()noexcept->void;
	protected:
		static sp<IWorker> _pInstance;
		up<jthread> _pThread;
		static std::atomic_flag _mutex;
	};
	struct Γ IPollWorker : IWorker, IPollster
	{
		IPollWorker( sv name )noexcept:IWorker{ name }{}
		virtual optional<bool> Poll()noexcept=0;
		void WakeUp()noexcept override;
		void Sleep()noexcept override;
	protected:
		void Run( stop_token st )noexcept override;
		atomic<TimePoint> _lastRequest{ Clock::now() };
	private:
		atomic<uint> _calls{0};
	};

	template<class TDerived>
	struct TWorker /*abstract*/: IPollWorker
	{
		TWorker()noexcept:IPollWorker{ TDerived::Name }{}
	protected:
		Ω Start()noexcept->sp<IWorker>;
	};

	template<class TArg, class TDerived>
	struct IQueueWorker /*abstract*/: TWorker<TDerived>
	{
		using base=TWorker<TDerived>; using Class=IQueueWorker<TArg,TDerived>;
		Ω Push( TArg&& x )noexcept->void;
		β HandleRequest( TArg&& x )noexcept->void=0;
		β SetWorker( TArg& x )noexcept->void=0;
	protected:
		α Poll()noexcept->optional<bool>  override;
		α Queue()noexcept->QueueMove<TArg>&{ return _queue;}
	private:
		static sp<TDerived> Start()noexcept{ return dynamic_pointer_cast<TDerived>(base::Start()); }
		QueueMove<TArg> _queue;
	};


	ⓣ TWorker<T>::Start()noexcept->sp<IWorker>
	{
		AtomicGuard l{ _mutex };
		if( !_pInstance )
		{
			//pInstance = _pInstance = make_shared<T>();
			//const bool addThreads = Settings::TryGet<uint8>( format("workers/{}/threads", T::Name) ).value_or( 0 );
			_pInstance = IApplication::AddPollster<T>();
			////var pSettings = Settings::TryGetSubcontainer<Settings::Container>(  );
			//if( addThreads )
			//	_pInstance->StartThread();
			//else
			//	//IApplication::AddP
			//	throw "not implemented";
			//_pInstance->_pThread = make_unique<jthread>([&]( stop_token st ){_pInstance->Run( st );} );
		}
		return _pInstance;
	}

#define $ template<class TArg, class TDerived> α IQueueWorker<TArg,TDerived>
	$::Push( TArg&& x )noexcept->void
	{
		if( auto p=Start(); p )
		{
			if( p->HasThread() )
				p->SetWorker( x );
//			else
//				IApplication::AddActiveWorker( p.get() );
			p->Queue().Push( move(x) );
		}
	}
	$::Poll()noexcept->optional<bool>
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
#undef $
}