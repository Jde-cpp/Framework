#include <jde/framework/process.h>
#include <signal.h>
#include <sys/types.h>
#include <stdexcept>

#include "Cache.h"
#include <jde/framework/io/file.h>
#include <jde/framework/settings.h>
#include "threading/InterruptibleThread.h"
#include "threading/Mutex.h"
#include "threading/Worker.h"
#include <jde/framework/collections/Vector.h>

#define let const auto

namespace Jde{
	constexpr ELogTags _tags = ELogTags::App;

	sp<IApplication> _pInstance;
	α IApplication::SetInstance( sp<IApplication> app )ι->void{ _pInstance = app; }
	α IApplication::Instance()ι->IApplication&{ return *_pInstance; }
	α IApplication::Kill( uint processId )ι->bool{return _pInstance ? _pInstance->KillInstance( processId ) : false;}

	string _applicationName;
	α Process::ApplicationName()ι->const string&{ return _applicationName; }

	bool _isConsole{};
	α Process::SetConsole( bool value )ι->void{ _isConsole=value;}
	α Process::IsConsole()ι->bool{ return _isConsole; }


	TimePoint _startTime{ Clock::now() };
	α Process::StartTime()ι->TimePoint{ return _startTime; };

	Vector<sp<Threading::InterruptibleThread>> _backgroundThreads;
	function<void()> OnExit;
}
namespace Jde{
	Vector<sp<void>> _keepAlives;
	α Process::AddKeepAlive( sp<void> pShared )ι->void{ _keepAlives.push_back( pShared ); }
	α Process::RemoveKeepAlive( sp<void> pShared )ι->void{
		_keepAlives.erase( pShared );
	}

	α Process::FindArg( string key )ι->optional<string>{
		auto p = Args().find( key );
		return p!=Args().end() ? p->second : optional<string>{};
	}
}
namespace Jde{
	vector<Threading::IPollWorker*> _activeWorkers; std::atomic_flag _activeWorkersMutex;

	const TimePoint _start=Clock::now();
	α IApplication::StartTime()ι->TimePoint{ return _start; }

	flat_set<string> IApplication::BaseStartup( int argc, char** argv, sv appName, string serviceDescription, optional<bool> console )ε{//no config file
		auto isConsole = console ? *console : Process::FindArg( "-c" ).has_value();
		Process::SetConsole( isConsole );
		{
			std::ostringstream os;
			os << "(" << OSApp::ProcessId() << ")";
			for( auto i=0; i<argc; ++i )
				os << argv[i] << " ";
			os << ";cwd=" << fs::current_path().string();
			//Logging::Default()->log( spdlog::source_loc{SRCE_CUR.file_name(),SRCE_CUR.line(),SRCE_CUR.function_name()}, (spdlog::level::level_enum)ELogLevel::Information, os.str() );
			Information{ ELogTags::App, "Starting {}{}", appName, os.str() };
		}
		_applicationName = appName;
		const string arg0{ argv[0] };
		bool terminate = !_debug;
		flat_set<string> values;
		for( int i=1; i<argc; ++i ){
			if( string(argv[i])=="-c" && !console )
				isConsole = true;
			else if( string(argv[i])=="-t" )
				terminate = !terminate;
			else if( string(argv[i])=="-install" ){
				Install( serviceDescription );
				throw Exception{ "successfully installed.", ELogLevel::Trace };
			}
			else if( string(argv[i])=="-uninstall" ){
				Uninstall();
				throw Exception{ "successfully uninstalled.", ELogLevel::Trace };
			}
			else
				values.emplace( argv[i] );
		}
		if( terminate )
			std::set_terminate( OnTerminate );
		if( isConsole )
			SetConsoleTitle( appName );
		else
			AsService();
		Logging::Initialize();
		SetThreadDscrptn( appName );
		_pInstance->AddSignals();
		return values;
	}

	std::condition_variable _workerCondition; mutex _workerConditionMutex;
	optional<int> _exitReason;
	bool _terminate{};
	α Process::ExitReason()ι->optional<int>{ return _exitReason; }
	α Process::SetExitReason( int reason, bool terminate )ι->void{ _exitReason = reason; _terminate = terminate; }
	α Process::ShuttingDown()ι->bool{ return (bool)_exitReason; }
	bool _finalizing{};
	α Process::Finalizing()ι->bool{ return _finalizing; }

	α Process::ExitException( exception&& e )ι->int{
		int y{ EXIT_FAILURE };
		auto cerrOutput = [&](){
			sv prefix = y==0 ? "Exiting on exception:  " : "Exiting on error:  ";
			std::cerr << prefix << e.what() << std::endl;
		};
		if( auto p = dynamic_cast<IException*>(&e); p ){
			y = p->Level()==ELogLevel::Trace ? EXIT_SUCCESS :
				p->Code ? (int)p->Code : EXIT_FAILURE;
		}
		cerrOutput();
		return y;
	}

	up<vector<function<void(bool)>>> _pShutdownFunctions;
	α Process::AddShutdownFunction( function<void(bool)>&& shutdown )ι->void{
		if( !_pShutdownFunctions )
			_pShutdownFunctions = mu<vector<function<void(bool)>>>();
		_pShutdownFunctions->push_back( shutdown );
	}

	static Vector<sp<IShutdown>> _shutdowns;
	α Process::AddShutdown( sp<IShutdown> pShared )ι->void{
		_shutdowns.push_back( move(pShared) );
	}
	α Process::RemoveShutdown( sp<IShutdown> pShutdown )ι->void{
		_shutdowns.erase( pShutdown );
	}

	Vector<IShutdown*> _rawShutdowns;
	α Process::AddShutdown( IShutdown* shutdown )ι->void{
		ASSERT( !_rawShutdowns.find(shutdown) );
		_rawShutdowns.push_back( shutdown );
	}
	α Process::RemoveShutdown( IShutdown* pShutdown )ι->void{
		ASSERT( _rawShutdowns.find(pShutdown) );
		_rawShutdowns.erase( pShutdown );
	}

	//TODOThreading remove this
	α IApplication::AddActiveWorker( Threading::IPollWorker* p )ι->void{
#ifndef _MSC_VER
		ASSERT( false );
#endif
		AtomicGuard l{ _activeWorkersMutex };
		if( find(_activeWorkers.begin(), _activeWorkers.end(), p)==_activeWorkers.end() ){
			_activeWorkers.push_back( p );
			if( _activeWorkers.size()==1 ){
				unique_lock<mutex> lk( _workerConditionMutex );
				l.unlock();
				OSApp::UnPause();
				_workerCondition.notify_one();
			}
		}
	}
	α IApplication::RemoveActiveWorker( Threading::IPollWorker* p )ι->void{
		AtomicGuard l{ _activeWorkersMutex };
		_activeWorkers.erase( remove(_activeWorkers.begin(), _activeWorkers.end(), p), _activeWorkers.end() );
	}

	α Process::Pause()ι->int{
		Information{ ELogTags::App, "Pausing main thread." };
		while( !_exitReason ){
			AtomicGuard l{ _activeWorkersMutex };
			uint size = _activeWorkers.size();
			if( size ){
				l.unlock();
				bool processed = false;
				for( uint i=0; i<size; ++i ){
					AtomicGuard l2{ _activeWorkersMutex };
					auto p = i<_activeWorkers.size() ? _activeWorkers[i] : nullptr; if( !p ) break;
					l2.unlock();
					if( let pWorkerProcessed = p->Poll();  pWorkerProcessed )
						processed = *pWorkerProcessed || processed;
					else
						IApplication::RemoveActiveWorker( p );
				}
				if( !processed )
					std::this_thread::yield();
			}
			else{
				l.unlock();
				OSApp::Pause();
			}
		}
		Information{ _tags, "Pause returned = {}.", _exitReason ? std::to_string(_exitReason.value()) : "null" };
		_backgroundThreads.visit( [](auto&& p){ p->Interrupt(); } );
		Shutdown( _exitReason.value_or(-1) );
		return _exitReason.value_or( -1 );
	}

	α Cleanup()ι->void;
	α Process::Shutdown( int exitReason )ι->void{
		bool terminate{ false }; //use case might be if non-terminate took too long
		SetExitReason( exitReason, terminate );//Sets ShuttingDown should be called in OnExit handler
		_shutdowns.erase( [terminate](auto& p){
			p->Shutdown( terminate );
		});
		Information{ ELogTags::App | ELogTags::Shutdown, "[{}]Waiting for process to complete. exitReason: {}, terminate: {}, background threads: {}", OSApp::ProcessId(), _exitReason.value(), terminate, _backgroundThreads.size() };
		while( _backgroundThreads.size() ){
			_backgroundThreads.erase_if( [](let& p)->bool {
				auto done = p->IsDone();
				if( done )
					p->Join();
				else if( done = p->Id()==std::this_thread::get_id(); done )
					p->Detach();
				return done;
			});
			std::this_thread::yield(); //std::this_thread::sleep_for( 2s );
		}
		Debug{ ELogTags::App | ELogTags::Shutdown, "Background threads removed" };

		if( _pShutdownFunctions )
			for_each( *_pShutdownFunctions, [=](let& shutdown){ shutdown( terminate ); } );
		Debug{ ELogTags::App | ELogTags::Shutdown, "Shutdown functions removed" };
		_rawShutdowns.erase( [=](auto& p){ p->Shutdown( terminate );} );
		Debug{ ELogTags::App | ELogTags::Shutdown, "Raw functions removed" };
		Cleanup();
		_finalizing = true;
	}
	α IApplication::AddThread( sp<Threading::InterruptibleThread> pThread )ι->void{
		Trace{ _tags, "Adding Backgound thread" };
		_backgroundThreads.push_back( pThread );
	}

	α IApplication::RemoveThread( sv name )ι->sp<Threading::InterruptibleThread>{
		sp<Threading::InterruptibleThread> pThread;
		_backgroundThreads.erase_if( [name,&pThread](let& p){
			bool equal = p->Name==name;
			if( equal )
				pThread = p;
			return equal;
		});
		return pThread;
	}

	α IApplication::RemoveThread( sp<Threading::InterruptibleThread> pThread )ι->void{
		_backgroundThreads.erase( pThread );
	}


	α Cleanup()ι->void{
		_pInstance = nullptr;
		_pShutdownFunctions = nullptr;
		Information( ELogTags::App, "Clearing Logger" );
		Logging::DestroyLoggers();
	}
	α IApplication::ApplicationDataFolder()ι->fs::path{
		return ProgramDataFolder()/OSApp::CompanyRootDir()/Process::ProductName();
	}
}