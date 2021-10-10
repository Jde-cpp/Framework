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
		β Initialize()noexcept->void;
		α Shutdown()noexcept->void override;
		α HasThread()noexcept->bool{ return _pThread!=nullptr; }
		virtual void Run( stop_token st )noexcept;
		sv Name;
		const uint8 ThreadCount;
		α StartThread()noexcept->void;
	protected:
		α Settings()->optional<Settings::Container>{ return Settings::TryGetSubcontainer<Settings::Container>( "workers", Name ); }
		static sp<IWorker> _pInstance;
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

	template<class TArg, class TDerived>
	struct IQueueWorker /*abstract*/: TWorker<TDerived>
	{
		using base=TWorker<TDerived>; using Class=IQueueWorker<TArg,TDerived>;
		IQueueWorker( sv threadName ):base{ threadName }{}
		Ω Push( TArg&& x )noexcept->void;
		virtual α HandleRequest( TArg&& x )noexcept->void=0;
		virtual α SetWorker( TArg& x )noexcept->void=0;
	protected:
		α Poll()noexcept->optional<bool>  override;
		α Queue()noexcept->QueueMove<TArg>&{ return _queue;}
	private:
		static sp<TDerived> Start()noexcept{ return dynamic_pointer_cast<TDerived>(base::Start()); }
		QueueMove<TArg> _queue;
	};

	
	ⓣ TWorker<T>::Start()noexcept->sp<IWorker>
	{
		if( IApplication::ShuttingDown() )
			return {};
		Threading::AtomicGuard l{ _mutex };
		if( !_pInstance )
		{
			_pInstance = make_shared<T>();
			IApplication::AddShutdown( _pInstance );
			var pSettings = Settings::TryGetSubcontainer<Settings::Container>( "workers", _pInstance->Name );
			if( pSettings && pSettings->TryGet<uint8>("threads") )
				_pInstance->StartThread();
			else
				throw "not implemented";
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
			else
				IApplication::AddActiveWorker( p.get() );
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