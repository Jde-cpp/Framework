#include <jde/App.h>
#include <signal.h>
#include <sys/types.h>
#include <stdexcept>

#include "Cache.h"
#include <jde/io/File.h>
#include "log/server/ServerSink.h"
#include "Settings.h"
#include "threading/InterruptibleThread.h"

namespace Jde
{
	sp<IApplication> IApplication::_pInstance;
	unique_ptr<string> IApplication::_pApplicationName;

	mutex IApplication::_threadMutex;
	bool IApplication::_shuttingDown{false};
	VectorPtr<sp<Threading::InterruptibleThread>> IApplication::_pBackgroundThreads{ make_shared<std::vector<sp<Threading::InterruptibleThread>>>() };
	std::function<void()> OnExit;

	auto _pDeletedThreads = make_shared<std::vector<sp<Threading::InterruptibleThread>>>();

	std::vector<sp<void>> _objects;  mutex ObjectMutex;
	auto _pShutdowns = make_shared<std::vector<sp<IShutdown>>>();
}
#define var const auto

namespace Jde
{
	const TimePoint Start=Clock::now();
	TimePoint IApplication::StartTime()noexcept{ return Start; }

	IApplication::~IApplication()
	{
	}
	IO::DriveWorker _driveWorker;
	set<string> IApplication::BaseStartup( int argc, char** argv, sv appName, string serviceDescription/*, sv companyName*/ )noexcept(false)//no config file
	{
		{
			ostringstream os;
			for( uint i=0; i<argc; ++i )
				os << argv[i] << " ";
			_logger.log( spdlog::source_loc{FileName(MY_FILE).c_str(),__LINE__,__func__}, (spdlog::level::level_enum)ELogLevel::Information, os.str() );
		}
		_pApplicationName = std::make_unique<string>( appName );

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
				THROWX( Exception(ELogLevel::Trace, "successfully installed.") );
			}
			else if( string(argv[i])=="-uninstall" )
			{
				Uninstall();
				THROWX( Exception(ELogLevel::Trace, "successfully uninstalled.") );
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
		InitializeLogger();
		Threading::SetThreadDscrptn( appName );
		//INFO( "{}, settings='{}' cwd='{}' Running as console='{}'"sv, arg0, settingsPath, fs::current_path(), console );

		Cache::CreateInstance();
		_driveWorker.Initialize();
		_pInstance->AddSignals();
		return values;
	}

	vector<Threading::IPollWorker*> _activeWorkers; atomic<bool> _activeWorkersMutex;
	std::condition_variable _workerCondition; std::mutex _workerConditionMutex;
	int _exitReason{0};
	void IApplication::Exit( int reason )noexcept
	{
		_exitReason = reason;
		std::unique_lock<std::mutex> lk( _workerConditionMutex );
		_workerCondition.notify_one();
	}
	void IApplication::AddActiveWorker( Threading::IPollWorker* p )noexcept
	{
		Threading::AtomicGuard l{ _activeWorkersMutex };
		if( find(_activeWorkers.begin(), _activeWorkers.end(), p)==_activeWorkers.end() )
		{
			_activeWorkers.push_back( p );
			if( _activeWorkers.size()==1 )
			{
				std::unique_lock<std::mutex> lk( _workerConditionMutex );
				l.unlock();
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
		INFO( "Pausing main thread. {}"sv, OSApp::ProcessId() );
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
				std::unique_lock<std::mutex> lk( _workerConditionMutex );
				l.unlock();
				_workerCondition.wait( lk );
			}
		}
		//TODO wait for signal
		INFO( "Pause returned - {}."sv, _exitReason );
		lock_guard l{_threadMutex};
		for( auto& pThread : *_pBackgroundThreads )
			pThread->Interrupt();
		IApplication::Shutdown();
	}

	void IApplication::Shutdown()noexcept
	{
		_shuttingDown = true;
		INFO( "Waiting for process to complete. {}"sv, OSApp::ProcessId() );
		GarbageCollect();
		{
			lock_guard l{ObjectMutex};
			for( auto pShutdown : *_pShutdowns )
			{
				if( pShutdown )//not sure why it would be null.
					pShutdown->Shutdown();
			}
			_pShutdowns->clear();
			_pShutdowns = nullptr;
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

	void IApplication::RemoveThread( sp<Threading::InterruptibleThread> pThread )noexcept
	{
		TRACE( "RemoveThread"sv );
		lock_guard l{_threadMutex};
		_pDeletedThreads->push_back( pThread );//TODO remove from _pBackgroundThreads
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
		lock_guard l{ ObjectMutex };
		_objects.push_back( pShared );
	}

	void IApplication::AddShutdown( sp<IShutdown> pShared )noexcept
	{
		Add( pShared );
		lock_guard l{ ObjectMutex };
		_pShutdowns->push_back( pShared );
	}
	void IApplication::RemoveShutdown( sp<IShutdown> pShutdown )noexcept
	{
		Remove( pShutdown );
		lock_guard l{ ObjectMutex };
		if( auto p=find( _pShutdowns->begin(), _pShutdowns->end(), pShutdown ); p!=_pShutdowns->end() )
			_pShutdowns->erase( p );
		else
			WARN( "Could not find shutdown"sv );
	}

	void IApplication::Remove( sp<void> pShared )noexcept
	{
		lock_guard l{ObjectMutex};
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
		//if( _pServerSink )
		//	_pServerSink->Destroy();
		Jde::DestroyLogger();
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