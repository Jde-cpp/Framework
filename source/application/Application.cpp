#include "../pc.h"//stdafx.h
#include "Application.h"
#include <signal.h>
#include <sys/types.h>
#include <stdexcept>

#include "../Cache.h"
#include "../log/server/ServerSink.h"
#include "../Settings.h"
#include "../threading/InterruptibleThread.h"

namespace Jde
{
	sp<IApplication> IApplication::_pInstance;
	unique_ptr<string> IApplication::_pApplicationName;
	unique_ptr<string> IApplication::_pCompanyName;

	mutex IApplication::_threadMutex;
	bool IApplication::_shuttingDown{false};
	VectorPtr<sp<Threading::InterruptibleThread>> IApplication::_pBackgroundThreads{ make_shared<std::vector<sp<Threading::InterruptibleThread>>>() };
	std::function<void()> OnExit;

	auto _pDeletedThreads = make_shared<std::vector<sp<Threading::InterruptibleThread>>>();

	auto _pObjects = make_shared<std::list<sp<void>>>();  mutex ObjectMutex;
	auto _pShutdowns = make_shared<std::vector<sp<IShutdown>>>();
}
#define var const auto

namespace Jde
{
	const TimePoint Start=Clock::now();
	TimePoint IApplication::StartTime()noexcept{ return Start; }

	IApplication::~IApplication()
	{
		if( HaveLogger() )
			DBG0( "IApplication::~IApplication"sv );
	}
	set<string> IApplication::BaseStartup( int argc, char** argv, string_view appName, string_view companyName )noexcept(false)//no config file
	{
		_pApplicationName = std::make_unique<string>( appName );
		_pCompanyName = std::make_unique<string>(companyName);

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
			else
				values.emplace( argv[i] );
		}
		if( terminate )
			std::set_terminate( OnTerminate );
		if( !console )
			AsService();
		var fileName = std::filesystem::path{ format("{}.json", appName) };
		//std::filesystem::path settingsPath = ProgramDataFolder()/companyName/appName/fileName;
		std::filesystem::path settingsPath{ fileName };
		if( !fs::exists(settingsPath) )
		{
			settingsPath = std::filesystem::path{".."}/fileName;
			if( !fs::exists(settingsPath) )
				settingsPath = ApplicationDataFolder()/fileName;
		}
		Settings::SetGlobal( std::make_shared<Jde::Settings::Container>(settingsPath) );
		InitializeLogger( appName );
		SetConsoleTitle( appName );
		Threading::SetThreadDscrptn( string(appName) );
		INFO( "{}, settings='{}' Running as console='{}'"sv, arg0, settingsPath, console );

		Cache::CreateInstance();
		return values;
	}
	// bool IApplication::Kill( uint processId )noexcept
	// {
	// 	return _pInstance ? _pInstance->KillInstance( processId ) : false;
	// }
	void IApplication::Pause()noexcept
	{
		_pInstance->AddSignals();
		_pInstance->OSPause();
	}

	void IApplication::Wait()noexcept
	{
		_shuttingDown = true;
		INFO( "Waiting for process to complete. {}"sv, ProcessId() );
		GarbageCollect();
		{
			lock_guard l{ObjectMutex};
			for( auto pShutdown : *_pShutdowns )
			{
		//		if( pShutdown.use_count()>4 )//1 pShutdown, 1 class, 1 _pShutdowns, 1 _pObjects
		//			CRITICAL( "Use Count=={}"sv, pShutdown.use_count() );
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
				//DBGX( "{} = {}", _pBackgroundThreads->size(), _pBackgroundThreads->size()==0 );
				if( _pBackgroundThreads->size()==0 )
					break;
			}
			std::this_thread::sleep_for( 2s );
		}
		Settings::SetGlobal( nullptr );
		DBG0( "Leaving Application::Wait"sv );
	}

	void IApplication::AddThread( sp<Threading::InterruptibleThread> pThread )noexcept
	{
		TRACE0( "Adding Backgound thread"sv );
		lock_guard l{_threadMutex};
		_pBackgroundThreads->push_back( pThread );
	}

	void IApplication::RemoveThread( sp<Threading::InterruptibleThread> pThread )noexcept
	{
		TRACE0( "RemoveThread"sv );
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
		lock_guard l{ObjectMutex};
		_pObjects->push_back( pShared );
	}
	void IApplication::AddShutdown( sp<IShutdown> pShared )noexcept
	{
		Add( pShared );
		lock_guard l{ObjectMutex};
		_pShutdowns->push_back( pShared );
	}
	void IApplication::Remove( sp<void> pShared )noexcept
	{
		lock_guard l{ObjectMutex};
		for( auto ppObject = _pObjects->begin(); ppObject!=_pObjects->end(); ++ppObject )
		{
			if( *ppObject==pShared )
			{
				_pObjects->erase( ppObject );
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
		_pObjects = nullptr;
		_pBackgroundThreads = nullptr;
		_pDeletedThreads = nullptr;
		for( var& shutdown : *_pShutdownFunctions )
			shutdown();
		INFO0( "Clearing Logger"sv );
		if( GetServerSink() )
			GetServerSink()->Destroy();
		Jde::DestroyLogger();
		//can't work obviously INFO0( "~Clearing Logger"sv );
		_pApplicationName = nullptr;
		_pCompanyName = nullptr;
		_pInstance = nullptr;
		_pShutdownFunctions = nullptr;
#ifdef _MSC_VER
		_CrtDumpMemoryLeaks();
#endif
	}
}