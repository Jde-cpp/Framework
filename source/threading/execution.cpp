#include <jde/framework/thread/execution.h>
#include <jde/framework/settings.h>
#include <list>
#include <boost/asio.hpp>
#include "../threading/Thread.h"
#include <jde/framework/collections/Vector.h>

//#include <boost/asio/cancellation_signal.hpp>

#define let const auto
namespace Jde{
	namespace net = boost::asio;
	α ThreadCount()ι->int{ return std::max(Settings::FindNumber<int>( "/workers/executor" ).value_or(std::thread::hardware_concurrency()), 1); }
	sp<net::io_context> _ioc;
}
α Jde::Executor()ι->sp<net::io_context>{
	if( !_ioc )
		_ioc = ms<net::io_context>( ThreadCount() );
	return _ioc;
}

namespace Jde{
	static up<Vector<IShutdown*>> _shutdowns;
	α Execution::AddShutdown( IShutdown* shutdown )ι->void{
		if( !_shutdowns )
			_shutdowns = mu<Vector<IShutdown*>>();
	 	_shutdowns->push_back( shutdown );
	}

	struct CancellationSignals final{
		α Emit( net::cancellation_type ct = net::cancellation_type::all )ι->void;
		α Slot()ι->net::cancellation_slot;
		α Signal()ι->sp<net::cancellation_signal>;
		α Add( sp<net::cancellation_signal> s )ι->void{ lg _(_mtx); _sigs.push_back( s ); }
		α Clear()ι->void{ lg _(_mtx); _sigs.clear(); }
		α Size()ι->uint{ lg _(_mtx); return _sigs.size(); }
	private:
		vector<sp<net::cancellation_signal>> _sigs;
		mutex _mtx;
	};
	CancellationSignals _cancelSignals;
	α Execution::AddCancelSignal( sp<net::cancellation_signal> s )ι->void{ _cancelSignals.Add( s ); }

	α CancelSignals()->CancellationSignals&{ return _cancelSignals; }

	struct ExecutorContext final : IShutdown{
		ExecutorContext()ι:
			_thread{ [&](){Execute();} }{
			_started.wait( false );
			Information( ELogTags::App, "Created executor threadCount: {}.", ThreadCount() );
			Process::AddShutdown( this );
		}
		~ExecutorContext()ι{ if( !Process::Finalizing() ) Process::RemoveShutdown( this ); _cancelSignals.Clear(); }
		α Shutdown( bool terminate )ι->void override;
		Ω Started()ι->bool{ return _started.test(); }
		Ω Ioc()ι->sp<net::io_context>{ return _ioc; }
	private:
		Ω Execute()ι->void;
		std::jthread _thread;
		static atomic_flag _started;
	};
	up<ExecutorContext> _pExecutorContext;
	atomic_flag ExecutorContext::_started{};
	α Execution::Run()->void{
		if( !ExecutorContext::Started() ){
			auto keepAlive = Executor();
			_pExecutorContext = mu<ExecutorContext>();
		}
	}

	α CancellationSignals::Emit( net::cancellation_type ct )ι->void{
		lg _(_mtx);
		for( uint i=0; i<_sigs.size(); ++i ){
			Trace{ ELogTags::App, "Emitting cancellation signal {}.", i };
			_sigs[i]->emit( ct );
		}
	}
	α CancellationSignals::Slot()ι->net::cancellation_slot{
		return Signal()->slot();
	}
	α CancellationSignals::Signal()ι->sp<net::cancellation_signal>{
		lg _(_mtx);
		auto p = find_if( _sigs, [](auto& sig){ return !sig->slot().has_handler();} );
		return p == _sigs.end() ? _sigs.emplace_back( ms<net::cancellation_signal>() ) : *p;
	}
	α ExecutorContext::Shutdown( bool terminate )ι->void{
		Debug( ELogTags::App, "Executor Shutdown: instances: {}.", _ioc.use_count() );
		if( _shutdowns )
			_shutdowns->erase( [=](auto p){p->Shutdown(terminate);} );
		if( _ioc && terminate )
			_ioc->stop(); // Stop the `io_context`. This will cause `run()` to return immediately, eventually destroying the `io_context` and all of the sockets in it.
		else
			_cancelSignals.Emit( net::cancellation_type::all );

		//Process::RemoveShutdown( this ); deadlock
	}
	α ExecutorContext::Execute()ι->void{
		Threading::SetThreadDscrptn( "Ex[0]" );
		std::vector<std::jthread> v; v.reserve( ThreadCount() - 1 );
		auto ioc = _ioc; //keep alive
		for( auto i = ThreadCount() - 1; i > 0; --i )
			v.emplace_back( [=]{ Threading::SetThreadDscrptn( Ƒ("Ex[{}]", i) ); ioc->run(); } );
		Trace( ELogTags::App, "Executor Started: instances: {}.", ioc.use_count() );
		_started.test_and_set();
		_started.notify_all();
		ioc->run();
		_ioc.reset();
		_started.clear();
		if( _shutdowns )
			_shutdowns->erase( [=](auto p){ p->Shutdown( false ); } );
		Information( ELogTags::App, "Executor Stopped: instances: {}.", ioc.use_count() );
		for( auto& t : v )
			t.join();
		Debug( ELogTags::App, "Removing Executor remaining instances: {}.", ioc.use_count()-1 );
		ioc.reset(); //need to clear out client connections.
		_cancelSignals.Clear();
		_started.clear();
	}

}
