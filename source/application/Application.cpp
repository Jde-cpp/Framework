#include "stdafx.h"
#include "Application.h"
#include <signal.h>
#include <sys/types.h>
#include <stdexcept>
#include <execinfo.h>

#include "../Diagnostics.h"
#include "../Cache.h"
#include "../log/server/ServerSink.h"
//#include "db/Database.h"
#include "../Settings.h"
#include "../threading/InterruptibleThread.h"

namespace Jde
{
	sp<IApplication> IApplication::_pInstance;
	mutex IApplication::_threadMutex;
	sp<std::list<sp<Threading::InterruptibleThread>>> IApplication::_pBackgroundThreads{ make_shared<std::list<sp<Threading::InterruptibleThread>>>() };
//	bool Stop{false};
	std::function<void()> OnExit;
	
	auto _pDeletedThreads = make_shared<std::list<sp<Threading::InterruptibleThread>>>(); 

	auto _pObjects = make_shared<std::list<sp<void>>>();  mutex ObjectMutex;
	auto _pShutdowns = make_shared<std::list<sp<IShutdown>>>();
}

#ifdef _MSC_VER
//	#include "ApplicationWindows.cpp"
#else
//	#include "ApplicationLinux.cpp"
#endif
#define var const auto
//using namespace std::chrono_literals;

namespace Jde
{
	const TimePoint Start=Clock::now();
	TimePoint IApplication::StartTime()noexcept{ return Start; }
	void OnTerminate()
	{
		void *trace_elems[20];
		auto trace_elem_count( backtrace(trace_elems, 20) );
		char **stack_syms( backtrace_symbols(trace_elems, trace_elem_count) );
		ostringstream os;
		for( auto i = 0; i < trace_elem_count ; ++i )
			os << stack_syms[i] << std::endl;

		IApplication::AddApplicationLog( ELogLevel::Critical, os.str() );
		free( stack_syms );
		exit( EXIT_FAILURE );
	}
	IApplication::~IApplication()
	{
		if( HaveLogger() )
			DBG0( "IApplication::~IApplication" );
	}
	void IApplication::BaseStartup( int argc, char** argv, string_view appName )noexcept
	{
		bool console = false;
		for( int i=1; i<argc; ++i )
		{
			if( string(argv[i])=="-c" )
				console = true;
		}
		std::set_terminate( OnTerminate );

		if( !console )
			AsService();
		var settingsPath = std::filesystem::path( fmt::format("{}.json", appName) );
		Settings::SetGlobal( std::make_shared<Jde::Settings::Container>(settingsPath) );
		InitializeLogger( appName );
		SetConsoleTitle( appName );
		Jde::Threading::SetThreadDescription( string(appName) );
		INFO( "{}, settings='{}' Running as console='{}'", appName, settingsPath, console );

		Cache::CreateInstance();
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
		//AddSignals();
		INFO( "Waiting for process to complete. {}", Diagnostics::ProcessId() );
		GarbageCollect();
		{
			lock_guard l{ObjectMutex};
			for( auto pShutdown : *_pShutdowns )
				pShutdown->Shutdown();
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
		DBG0( "Leaving Application::Wait" );
	}
	
	void IApplication::AddThread( sp<Threading::InterruptibleThread> pThread )noexcept
	{
		TRACE0( "Adding Backgound thread" );
		lock_guard l{_threadMutex};
		_pBackgroundThreads->push_back( pThread );
	}

	void IApplication::RemoveThread( sp<Threading::InterruptibleThread> pThread )noexcept
	{
		TRACE0( "RemoveThread" );
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
			ppThread = _pBackgroundThreads->erase( ppThread );
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
	vector<function<void()>> _shutdowns;
	void IApplication::AddShutdownFunction( function<void()>&& shutdown )noexcept
	{
		_shutdowns.push_back( shutdown );
	}
	void IApplication::CleanUp()noexcept
	{
		_pObjects = nullptr;
		_pBackgroundThreads = nullptr;
		_pDeletedThreads = nullptr;
		//DB::CleanDataSources();  TODO ReAdd when adding a data source.
		for( var shutdown : _shutdowns )
			shutdown();
		INFO0( "Clearing Logger" );
		if( GetServerSink() )
			GetServerSink()->Destroy();
		Jde::DestroyLogger();
#ifdef _MSC_VER
		_CrtDumpMemoryLeaks();
#endif
	}
}