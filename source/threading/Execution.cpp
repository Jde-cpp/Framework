#include <jde/thread/Execution.h>
#include <list>
#include <boost/asio.hpp>
#include "../threading/Thread.h"
#include "../../../Framework/source/collections/Vector.h"

//#include <boost/asio/cancellation_signal.hpp>

#define var const auto
namespace Jde{
	namespace net = boost::asio;
	const int _threadCount{ std::max( Settings::Get<int>( "workers/executor" ).value_or(std::thread::hardware_concurrency()), 1 ) };
	sp<net::io_context> _ioc = ms<net::io_context>( _threadCount );
}
α Jde::Executor()ι->sp<net::io_context>{ return _ioc; }

namespace Jde{
	static up<Vector<IShutdown*>> _shutdowns;
	α Execution::AddShutdown( IShutdown* pShutdown )ι->void{
		if( !_shutdowns )
			_shutdowns = mu<Vector<IShutdown*>>();
	 	_shutdowns->push_back( pShutdown );
	}

	struct CancellationSignals final{
		α Emit( net::cancellation_type ct = net::cancellation_type::all )ι->void;
		α Slot()ι->net::cancellation_slot;
		α Signal()ι->sp<net::cancellation_signal>;
		α Add( sp<net::cancellation_signal> s )ι->void{ lg _(_mtx); _sigs.push_back( s ); }
	private:
		std::list<sp<net::cancellation_signal>> _sigs;
		mutex _mtx;
	};
	CancellationSignals _cancelSignals;
	α Execution::AddCancelSignal( sp<net::cancellation_signal> s )ι->void{ return _cancelSignals.Add( s ); }

	α CancelSignals()->CancellationSignals&{ return _cancelSignals; }

	struct ExecutorContext final : IShutdown{
		ExecutorContext()ι:
			_thread{ [&](){Execute();} }{
			_started.wait( false );
			Information( ELogTags::App, "Created executor threadCount: {}.", _threadCount );
			Process::AddShutdown( this );
		}
		~ExecutorContext()ι{ if( !Process::Finalizing() ) Process::RemoveShutdown( this ); }
		α Shutdown( bool terminate )ι->void override{
			Debug( ELogTags::App, "Executor Shutdown: instances: {}.", _ioc.use_count() );
			_shutdowns->erase( [=](auto p){ p->Shutdown( terminate ); } );
			if( _ioc && terminate )
				_ioc->stop(); // Stop the `io_context`. This will cause `run()` to return immediately, eventually destroying the `io_context` and all of the sockets in it.
			else
				_cancelSignals.Emit( net::cancellation_type::all );

			//Process::RemoveShutdown( this ); deadlock
		}
		Ω Started()ι->bool{ return _started.test(); }
		Ω Ioc()ι->sp<net::io_context>{ return _ioc; }
	private:
		Ω Execute()ι->void{
			Threading::SetThreadDscrptn( "Ex[0]" );
			std::vector<std::jthread> v; v.reserve( _threadCount - 1 );
			for( auto i = _threadCount - 1; i > 0; --i )
				v.emplace_back( [=]{ Threading::SetThreadDscrptn( Ƒ("Ex[{}]", i) ); _ioc->run(); } );
			Trace( ELogTags::App, "Executor Started: instances: {}.", _ioc.use_count() );
			_started.test_and_set();
			_started.notify_all();
			_ioc->run();
			_shutdowns->erase( [=](auto p){ p->Shutdown( false ); } );
			Information( ELogTags::App, "Executor Stopped: instances: {}.", _ioc.use_count() );
			_started.clear();
			for( auto& t : v )// (If we get here, it means we got a SIGINT or SIGTERM)
				t.join();
			Debug( ELogTags::App, "Removing Executor remaining instances: {}.", _ioc.use_count()-1 );
			_ioc.reset();
		}
		std::jthread _thread;
		static atomic_flag _started;
	};
	atomic_flag ExecutorContext::_started{};
	up<ExecutorContext> _pExecutorContext;
	α Execution::Run()->void{ if( !_pExecutorContext ) _pExecutorContext = mu<ExecutorContext>(); }

	α CancellationSignals::Emit( net::cancellation_type ct )ι->void{
		lg _(_mtx);
		for( auto & sig : _sigs )
			sig->emit( ct );
	}
	α CancellationSignals::Slot()ι->net::cancellation_slot{
		return Signal()->slot();
	}
	α CancellationSignals::Signal()ι->sp<net::cancellation_signal>{
		lg _(_mtx);
		auto p = find_if( _sigs, [](auto& sig){ return !sig->slot().has_handler();} );
		return p == _sigs.end() ? _sigs.emplace_back( ms<net::cancellation_signal>() ) : *p;
	}
}
