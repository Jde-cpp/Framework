﻿#include <jde/App.h>
#include <signal.h>
#include <sys/types.h>
#include <stdexcept>

#include "Cache.h"
#include <jde/io/File.h>
#include "Settings.h"
#include "threading/InterruptibleThread.h"

#define var const auto

namespace Jde{
	static sp<LogTag> _logTag = Logging::Tag( "app" );
	α AppTag()ι->sp<LogTag>{ return _logTag; }


	sp<IApplication> IApplication::_pInstance;
	up<string> IApplication::_pApplicationName;

	mutex IApplication::_threadMutex;
	VectorPtr<sp<Threading::InterruptibleThread>> IApplication::_pBackgroundThreads{ make_shared<vector<sp<Threading::InterruptibleThread>>>() };
	function<void()> OnExit;

	auto _pDeletedThreads = make_shared<vector<sp<Threading::InterruptibleThread>>>();

	vector<sp<void>> IApplication::_objects; mutex IApplication::_objectMutex;
	vector<Threading::IPollWorker*> IApplication::_activeWorkers; std::atomic_flag IApplication::_activeWorkersMutex;
	vector<sp<IShutdown>> IApplication::_shutdowns;

	const TimePoint Start=Clock::now();
	TimePoint IApplication::StartTime()ι{ return Start; }

	flat_set<string> IApplication::BaseStartup( int argc, char** argv, sv appName, string serviceDescription/*, sv companyName*/ )ε{//no config file
		{
			ostringstream os;
			os << "(" << OSApp::ProcessId() << ")";
			for( auto i=0; i<argc; ++i )
				os << argv[i] << " ";
			Logging::Default()->log( spdlog::source_loc{FileName(SRCE_CUR.file_name()).c_str(),SRCE_CUR.line(),SRCE_CUR.function_name()}, (spdlog::level::level_enum)ELogLevel::Information, os.str() ); //TODO add cwd.
		}
		_pApplicationName = mu<string>( appName );

		bool console = false;
		const string arg0{ argv[0] };
		bool terminate = !_debug;
		flat_set<string> values;
		for( int i=1; i<argc; ++i ){
			if( string(argv[i])=="-c" )
				console = true;
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
		if( console )
			SetConsoleTitle( appName );
		else
			AsService();
		Logging::Initialize();
		Threading::SetThreadDscrptn( appName );

		//_driveWorker.Initialize();
		_pInstance->AddSignals();
		return values;
	}

	std::condition_variable _workerCondition; mutex _workerConditionMutex;
	optional<int> _exitReason;
	α IApplication::Exit( int reason )ι->void{ _exitReason = reason; }
	α IApplication::ShuttingDown()ι->bool{ return (bool)_exitReason; }

	α IApplication::AddActiveWorker( Threading::IPollWorker* p )ι->void{
#ifndef _MSC_VER
		ASSERT( false );//need to implement signals in linux for _workerCondition.
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
	
	α IApplication::Pause()ι->int{
		INFO( "Pausing main thread." );
		while( !_exitReason ){
			AtomicGuard l{ _activeWorkersMutex };
			uint size = _activeWorkers.size();
			if( size )
			{
				l.unlock();
				bool processed = false;
				for( uint i=0; i<size; ++i )
				{
					AtomicGuard l2{ _activeWorkersMutex };
					auto p = i<_activeWorkers.size() ? _activeWorkers[i] : nullptr; if( !p ) break;
					l2.unlock();
					if( var pWorkerProcessed = p->Poll();  pWorkerProcessed )
						processed = *pWorkerProcessed || processed;
					else
						RemoveActiveWorker( p );
				}
				if( !processed )
					std::this_thread::yield();
			}
			else
			{
				l.unlock();
				OSApp::Pause();
			}
		}
		INFO( "Pause returned = {}.", _exitReason ? std::to_string(_exitReason.value()) : "null" );
		{
			lg _{_threadMutex};
			for( auto& pThread : *_pBackgroundThreads )
				pThread->Interrupt();
		}
		IApplication::Shutdown( _exitReason.value_or(-1) );
		return _exitReason.value_or( -1 );
	}

	α IApplication::Shutdown( int exitReason )ι->void{
		Exit( exitReason );
		INFO( "({})Waiting for process to complete. {}", OSApp::ProcessId(), _exitReason.value() );
		GarbageCollect();
		{
			lg _{ _objectMutex };
			for( auto pShutdown : _shutdowns )
			{
				if( pShutdown )//not sure why it would be null.
					pShutdown->Shutdown();
			}
			_shutdowns.clear();
		}
		for(;;){
			{
				unique_lock l{_threadMutex};
				for( auto ppThread = _pBackgroundThreads->begin(); ppThread!=_pBackgroundThreads->end();  )
				{
					if( (*ppThread)->IsDone() )
					{
						(*ppThread)->Join();
						ppThread = _pBackgroundThreads->erase( ppThread );
					}
					else
						++ppThread;
				}
				if( _pBackgroundThreads->size()==0 )
					break;
			}
			std::this_thread::yield(); //std::this_thread::sleep_for( 2s );
		}
		TRACE( "Leaving Application::Wait" );
	}
	α IApplication::AddThread( sp<Threading::InterruptibleThread> pThread )ι->void{
		TRACE( "Adding Backgound thread" );
		lg _{_threadMutex};
		_pBackgroundThreads->push_back( pThread );
	}

	α IApplication::RemoveThread( sv name )ι->sp<Threading::InterruptibleThread>{
		lg _{_threadMutex};
		auto ppThread = find_if( _pBackgroundThreads->begin(), _pBackgroundThreads->end(), [name](var& p){ return p->Name==name;} );
		auto p = ppThread==_pBackgroundThreads->end() ? sp<Threading::InterruptibleThread>{} : *ppThread;
		if( ppThread!=_pBackgroundThreads->end() )
			_pBackgroundThreads->erase( ppThread );
		return p;
	}
	α IApplication::RemoveThread( sp<Threading::InterruptibleThread> pThread )ι->void{
		lg _{_threadMutex};
		_pDeletedThreads->push_back( pThread );
		for( auto ppThread = _pBackgroundThreads->begin(); ppThread!=_pBackgroundThreads->end(); ){
			if( *ppThread==pThread ){
				_pBackgroundThreads->erase( ppThread );
				break;
			}
			++ppThread;
		}
	}
	α IApplication::GarbageCollect()ι->void{
		lg _{_threadMutex};
		for( auto ppThread = _pDeletedThreads->begin(); ppThread!=_pDeletedThreads->end(); ){
			(*ppThread)->Join();
			ppThread = _pDeletedThreads->erase( ppThread );
		}
	}

	α IApplication::Add( sp<void> pShared )ι->void
	{
		lg _{ _objectMutex };
		_objects.push_back( pShared );
	}

	α IApplication::AddShutdown( sp<IShutdown> pShared )ι->void
	{
		lg _{ _objectMutex };
		//_objects.push_back( pShared ); not sure why objects would be added also.
		_shutdowns.push_back( move(pShared) );
	}
	α IApplication::RemoveShutdown( sp<IShutdown> pShutdown )ι->void
	{
		Remove( pShutdown );
		lg _{ _objectMutex };
		if( auto p=find( _shutdowns.begin(), _shutdowns.end(), pShutdown ); p!=_shutdowns.end() )
			_shutdowns.erase( p );
		else
			WARN( "Could not find shutdown" );
	}

	α IApplication::Remove( sp<void> pShared )ι->void
	{
		lg _{ _objectMutex };
		for( auto ppObject = _objects.begin(); ppObject!=_objects.end(); ++ppObject ){
			if( *ppObject==pShared ){
				_objects.erase( ppObject );
				break;
			}
		}
	}
	up<vector<function<void()>>> _pShutdownFunctions = mu<vector<function<void()>>>();
	α IApplication::AddShutdownFunction( function<void()>&& shutdown )ι->void{
		_pShutdownFunctions->push_back( shutdown );
	}
	α IApplication::Cleanup()ι->void{
		_pBackgroundThreads = nullptr;
		_pDeletedThreads = nullptr;
		for( var& shutdown : *_pShutdownFunctions )
			shutdown();
		INFO( "Clearing Logger" );
		Logging::DestroyLogger();
		_pApplicationName = nullptr;
		_pInstance = nullptr;
		_pShutdownFunctions = nullptr;
	}
	α IApplication::ApplicationDataFolder()ι->fs::path{
		return ProgramDataFolder()/OSApp::CompanyRootDir()/OSApp::ProductName();
	}
	α IApplication::IsConsole()ι->bool{
		return OSApp::Args().find( "-c" )!=OSApp::Args().end();
	}
}