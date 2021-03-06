#include <jde/App.h>
#include <signal.h>
#include <sys/types.h>
#include <stdexcept>

#include "Cache.h"
#include "log/server/ServerSink.h"
#include "Settings.h"
#include "threading/InterruptibleThread.h"

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

	auto _pObjects = make_shared<std::vector<sp<void>>>();  mutex ObjectMutex;
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
			DBG( "IApplication::~IApplication"sv );
	}
	set<string> IApplication::BaseStartup( int argc, char** argv, sv appName, sv companyName )noexcept(false)//no config file
	{
		_pApplicationName = std::make_unique<string>( appName );
		_pCompanyName = std::make_unique<string>( companyName );

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
		if( console )
			SetConsoleTitle( appName );
		else
			AsService();
		var fileName = std::filesystem::path{ format("{}.json", appName) };
		std::filesystem::path settingsPath{ fileName };
		if( !fs::exists(settingsPath) )
		{
			var settingsPathB = std::filesystem::path{".."}/fileName;
			settingsPath = fs::exists(settingsPathB) ? settingsPathB : ApplicationDataFolder()/fileName;
		}
		Settings::SetGlobal( std::make_shared<Jde::Settings::Container>(settingsPath) );
		InitializeLogger( appName );
		Threading::SetThreadDscrptn( appName );
		INFO( "{}, settings='{}' Running as console='{}'"sv, arg0, settingsPath, console );

		Cache::CreateInstance();
		return values;
	}
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
		Settings::SetGlobal( nullptr );
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
		_pObjects->push_back( pShared );
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
		INFO( "Clearing Logger"sv );
		if( GetServerSink() )
			GetServerSink()->Destroy();
		Jde::DestroyLogger();
		_pApplicationName = nullptr;
		_pCompanyName = nullptr;
		_pInstance = nullptr;
		_pShutdownFunctions = nullptr;
#ifdef _MSC_VER
		_CrtDumpMemoryLeaks();
#endif
	}
	fs::path IApplication::ApplicationDataFolder()noexcept
	{
#ifdef _MSC_VER
		sv company = CompanyName();
#else
		const string company{ format(".{}"sv, CompanyName()) };
#endif
		auto p=_pInstance;
		return p ? p->ProgramDataFolder()/company/ApplicationName() : ".app";
	}
}