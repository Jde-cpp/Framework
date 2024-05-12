#pragma once
#ifndef WORKER_H
#define WORKER_H
#include <jde/TypeDefs.h>
#include <jde/App.h>
#include "./jthread.h"
#include "./Mutex.h"
#include "../collections/Queue.h"
#include "InterruptibleThread.h"

namespace Jde::Threading{
	#define var const auto
#ifdef __clang__
	using Jde::stop_token;
#else
	using std::stop_token;
#endif
	/*handle signals, configuration*/
	struct Γ IWorker : IShutdown, std::enable_shared_from_this<IWorker>{
		IWorker( sv name )ι;
		virtual ~IWorker()=0;
		β Initialize()ι->void;
		α Shutdown()ι->void override;
		α HasThread()ι->bool{ return _pThread!=nullptr; }
		β Run( stop_token st )ι->void;
		sv NameInstance;
		const uint8 ThreadCount;
		α StartThread()ι->void;
	protected:
		static sp<IWorker> _pInstance;
		up<jthread> _pThread;
		static std::atomic_flag _mutex;
	};
	struct Γ IPollWorker : IWorker, IPollster{
		IPollWorker( sv name )ι:IWorker{ name }{}
		virtual optional<bool> Poll()ι=0;
		void WakeUp()ι override;
		void Sleep()ι override;
	protected:
		void Run( stop_token st )ι override;
		atomic<TimePoint> _lastRequest;
		α Calls()ι->uint{ return _calls; }
	private:
		atomic<uint> _calls{0};
	};

	template<class TDerived>
	struct TWorker /*abstract*/: IPollWorker{
		TWorker()ι:IPollWorker{ TDerived::Name }{}
	protected:
		Ω Start()ι->sp<IWorker>;
	};

	template<class TArg, class TDerived>
	struct IQueueWorker /*abstract*/: TWorker<TDerived>{
		using base=TWorker<TDerived>; using Class=IQueueWorker<TArg,TDerived>;
		Ω Push( TArg&& x )ι->void;
		β HandleRequest( TArg&& x )ι->void=0;
		β SetWorker( TArg& x )ι->void=0;
	protected:
		α Poll()ι->optional<bool>  override;
		α Queue()ι->QueueMove<TArg>&{ return _queue;}
	private:
		static sp<TDerived> Start()ι{ return dynamic_pointer_cast<TDerived>(base::Start()); }
		QueueMove<TArg> _queue;
	};


	Ŧ TWorker<T>::Start()ι->sp<IWorker>{
		AtomicGuard l{ _mutex };
		if( !_pInstance || !_pInstance->HasThread() )
		{
			//pInstance = _pInstance = make_shared<T>();
			//const bool addThreads = Settings::TryGet<uint8>( Jde::format("workers/{}/threads", T::Name) ).value_or( 0 );
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
	$::Push( TArg&& x )ι->void
	{
		if( auto p=Start(); p )
		{
			if( p->HasThread() )
				p->SetWorker( x ); //keepalive
//			else
//				IApplication::AddActiveWorker( p.get() );
			p->Queue().Push( move(x) );
		}
	}
	$::Poll()ι->optional<bool>
	{
		bool handled = false;
		if( auto p = _queue.Pop(); p )
		{
			HandleRequest( std::move(*p) );
			handled = true;
		}
		return handled;
	}
}
#undef TARG
#undef var
#undef $
#endif