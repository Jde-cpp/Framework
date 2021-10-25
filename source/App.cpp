#include <jde/App.h>
#include <signal.h>
#include <sys/types.h>
#include <stdexcept>

#include "Cache.h"
#include <jde/io/File.h>
#include "log/server/ServerSink.h"
#include "Settings.h"
#include "threading/InterruptibleThread.h"

#define var const auto

namespace Jde
{
	sp<IApplication> IApplication::_pInstance;
	unique_ptr<string> IApplication::_pApplicationName;

	mutex IApplication::_threadMutex;
	VectorPtr<sp<Threading::InterruptibleThread>> IApplication::_pBackgroundThreads{ make_shared<vector<sp<Threading::InterruptibleThread>>>() };
	function<void()> OnExit;

	auto _pDeletedThreads = make_shared<vector<sp<Threading::InterruptibleThread>>>();

	//auto _pShutdowns = make_shared<vector<sp<IShutdown>>>();
	vector<sp<void>> IApplication::_objects; mutex IApplication::_objectMutex;
	vector<Threading::IPollWorker*> IApplication::_activeWorkers; atomic<bool> IApplication::_activeWorkersMutex;
	vector<sp<IShutdown>> IApplication::_shutdowns;

	const TimePoint Start=Clock::now();
	TimePoint IApplication::StartTime()noexcept{ return Start; }

	IApplication::~IApplication()
	{
	}
	//IO::DriveWorker _driveWorker;
	set<string> IApplication::BaseStartup( int argc, char** argv, sv appName, string serviceDescription/*, sv companyName*/ )noexcept(false)//no config file
	{
		{
			ostringstream os;
			os << "(" << OSApp::ProcessId() << ")";
			for( uint i=0; i<argc; ++i )
				os << argv[i] << " ";
			Logging::Default().log( spdlog::source_loc{FileName(MY_FILE).c_str(),__LINE__,__func__}, (spdlog::level::level_enum)ELogLevel::Information, os.str() ); //TODO add cwd.
		}
		_pApplicationName = make_unique<string>( appName );

		bool console = false;
		const string arg0{ argv[0] };
#ifdef NDEBUG
		bool terminate = true;
#else
		bool terminate = false;
#endif
		set<string> values;
		for( int i=1; i<argc; ++i )
		{
			if( string(argv[i])=="-c" )
				console = true;
			else if( string(argv[i])=="-t" )
				terminate = !terminate;
			else if( string(argv[i])=="-install" )
			{
				Install( serviceDescription );
				throw Exception{ ELogLevel::Trace, "successfully installed." };
			}
			else if( string(argv[i])=="-uninstall" )
			{
				Uninstall();
				throw Exception{ ELogLevel::Trace, "successfully uninstalled." };
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
	int _exitReason{0};
	α IApplication::Exit( int reason )noexcept->void{ _exitReason = reason; }
	α IApplication::ShuttingDown()noexcept->bool{ return _exitReason; }

	α IApplication::AddActiveWorker( Threading::IPollWorker* p )noexcept->void
	{
#ifndef _MSC_VER
		ASSERT( false );//need to implement signals in linux for _workerCondition.
#endif
		Threading::AtomicGuard l{ _activeWorkersMutex };
		if( find(_activeWorkers.begin(), _activeWorkers.end(), p)==_activeWorkers.end() )
		{
			_activeWorkers.push_back( p );
			if( _activeWorkers.size()==1 )
			{
				unique_lock<mutex> lk( _workerConditionMutex );
				l.unlock();
				OSApp::UnPause();
				_workerCondition.notify_one();
			}
		}
	}
	void IApplication::RemoveActiveWorker( Threading::IPollWorker* p )noexcept
	{
		Threading::AtomicGuard l{ _activeWorkersMutex };
		_activeWorkers.erase( remove(_activeWorkers.begin(), _activeWorkers.end(), p), _activeWorkers.end() );
	}
	void IApplication::Pause()noexcept
	{
		INFO( "Pausing main thread." );
		while( !_exitReason )
		{
			Threading::AtomicGuard l{ _activeWorkersMutex };
			uint size = _activeWorkers.size();
			if( size )
			{
				l.unlock();
				bool processed = false;
				for( uint i=0; i<size; ++i )
				{
					Threading::AtomicGuard l2{ _activeWorkersMutex };
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
				// unique_lock<std::mutex> lk( _workerConditionMutex );
				// 
				// _workerCondition.wait( lk );
			}
		}
		//TODO wait for signal
		INFO( "Pause returned - {}.", _exitReason );
		{
			lock_guard l{_threadMutex};
			for( auto& pThread : *_pBackgroundThreads )
				pThread->Interrupt();
		}
		IApplication::Shutdown();
	}

	void IApplication::Shutdown()noexcept
	{
		INFO( "Waiting for process to complete. {}"sv, OSApp::ProcessId() );
		GarbageCollect();
		{
			lock_guard l{ _objectMutex };
			for( auto pShutdown : _shutdowns )
			{
				if( pShutdown )//not sure why it would be null.
					pShutdown->Shutdown();
			}
			_shutdowns.clear();
		}
		for(;;)
		{
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
		DBG( "Leaving Application::Wait"sv );
	}
	void IApplication::AddThread( sp<Threading::InterruptibleThread> pThread )noexcept
	{
		TRACE( "Adding Backgound thread"sv );
		lock_guard l{_threadMutex};
		_pBackgroundThreads->push_back( pThread );
	}

	α IApplication::RemoveThread( sv name )noexcept->sp<Threading::InterruptibleThread>
	{
		lock_guard l{_threadMutex};
		auto ppThread = find_if( _pBackgroundThreads->begin(), _pBackgroundThreads->end(), [name](var& p){ return p->Name==name;} );
		auto p = ppThread==_pBackgroundThreads->end() ? sp<Threading::InterruptibleThread>{} : *ppThread;
		if( ppThread!=_pBackgroundThreads->end() )
			_pBackgroundThreads->erase( ppThread );
		return p;
	}
	α IApplication::RemoveThread( sp<Threading::InterruptibleThread> pThread )noexcept->void
	{
		lock_guard l{_threadMutex};
		_pDeletedThreads->push_back( pThread );
		for( auto ppThread = _pBackgroundThreads->begin(); ppThread!=_pBackgroundThreads->end();  )
		{
			if( *ppThread==pThread )
			{
				_pBackgroundThreads->erase( ppThread );
				break;
			}
			++ppThread;
		}
	}
	void IApplication::GarbageCollect()noexcept
	{
		lock_guard l{_threadMutex};
		for( auto ppThread = _pDeletedThreads->begin(); ppThread!=_pDeletedThreads->end(); )
		{
			(*ppThread)->Join();
			ppThread = _pDeletedThreads->erase( ppThread );
		}
	}

	void IApplication::Add( sp<void> pShared )noexcept
	{
		lock_guard l{ _objectMutex };
		_objects.push_back( pShared );
	}

	void IApplication::AddShutdown( sp<IShutdown> pShared )noexcept
	{
		lock_guard l{ _objectMutex };
		_objects.push_back( pShared );
		_shutdowns.push_back( pShared );
	}
	void IApplication::RemoveShutdown( sp<IShutdown> pShutdown )noexcept
	{
		Remove( pShutdown );
		lock_guard l{ _objectMutex };
		if( auto p=find( _shutdowns.begin(), _shutdowns.end(), pShutdown ); p!=_shutdowns.end() )
			_shutdowns.erase( p );
		else
			WARN( "Could not find shutdown"sv );
	}

	void IApplication::Remove( sp<void> pShared )noexcept
	{
		lock_guard l{ _objectMutex };
		for( auto ppObject = _objects.begin(); ppObject!=_objects.end(); ++ppObject )
		{
			if( *ppObject==pShared )
			{
				_objects.erase( ppObject );
				break;
			}
		}
	}
	up<vector<function<void()>>> _pShutdownFunctions = make_unique<vector<function<void()>>>();
	void IApplication::AddShutdownFunction( function<void()>&& shutdown )noexcept
	{
		_pShutdownFunctions->push_back( shutdown );
	}
	void IApplication::CleanUp()noexcept
	{
		_pBackgroundThreads = nullptr;
		_pDeletedThreads = nullptr;
		for( var& shutdown : *_pShutdownFunctions )
			shutdown();
		INFO( "Clearing Logger"sv );
		Logging::DestroyLogger();
		_pApplicationName = nullptr;
		_pInstance = nullptr;
		_pShutdownFunctions = nullptr;
	}
	α IApplication::ApplicationDataFolder()noexcept->fs::path
	{
		auto p=_pInstance;
		return p ? p->ProgramDataFolder()/OSApp::CompanyRootDir()/ApplicationName() : ".app";
	}
	α IApplication::IsConsole()noexcept->bool
	{
		return OSApp::Args().find( "-c" )!=OSApp::Args().end();
	}
}