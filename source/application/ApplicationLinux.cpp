#include "stdafx.h"
#include "ApplicationLinux.h"
	#include <syslog.h>
#include "../threading/InterruptibleThread.h"


#define var const auto
namespace Jde
{
	void IApplication::AddApplicationLog( ELogLevel level, const string& value )noexcept //called onterminate, needs to be static.
	{
		auto osLevel = LOG_DEBUG;
		if( level==ELogLevel::Debug )
			osLevel = LOG_INFO;
		else if( level==ELogLevel::Information )
			osLevel = LOG_NOTICE;
		else if( level==ELogLevel::Warning )
			osLevel = LOG_WARNING;
		else if( level==ELogLevel::Error )
			osLevel = LOG_ERR;
		else if( level==ELogLevel::Critical )
			osLevel = LOG_CRIT;
		syslog( osLevel, "%s",  value.c_str() );
	}
	void OSApp::Startup( int argc, char** argv, string_view appName )noexcept
	{
		IApplication::_pInstance = make_shared<OSApp>();
		IApplication::_pInstance->BaseStartup( argc, argv, appName );
	}

	void OSApp::OSPause()noexcept
	{ 
		INFO( "Pausing main thread. {}", getpid() );//[*** LOG ERROR ***] [console] [argument index out of range] [2018-04-20 06:47:33]
		pause(); 
		INFO0( "Pause returned." );
		IApplication::Wait();
	}

	bool OSApp::AsService()noexcept
	{
		return ::daemon( 1, 0 )==0;
	}

	void OSApp::ExitHandler( int s )
	{
	//	signal( s, SIG_IGN );
	//not supposed to log here...
		lock_guard l{_threadMutex};
		for( auto& pThread : *_pBackgroundThreads )
			pThread->Interrupt();
		//printf( "!!!!!!!!!!!!!!!!!!!!!Caught signal %d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",s );
		//pProcessManager->Stop();
		//delete pLogger; pLogger = nullptr;
		//exit( 1 ); 
	}

	bool OSApp::KillInstance( uint processId )noexcept
	{
		var result = ::kill( processId, 14 );
		if( result )
			ERR( "kill failed with '{}'.", result );
		else
			INFO( "kill sent to:  '{}'.", processId );
		return result==0;
	}

	void OSApp::AddSignals()noexcept
	{
/* 		struct sigaction sigIntHandler;//_XOPEN_SOURCE
		memset( &sigIntHandler, 0, sizeof(sigIntHandler) );
		sigIntHandler.sa_handler = ExitHandler;
		sigemptyset( &sigIntHandler.sa_mask );
		sigIntHandler.sa_flags = 0;*/
		signal( SIGINT, OSApp::ExitHandler );
		signal( SIGSTOP, OSApp::ExitHandler );
		signal( SIGKILL, OSApp::ExitHandler );
		signal( SIGTERM, OSApp::ExitHandler );
		signal( SIGALRM, OSApp::ExitHandler );
		//sigaction( SIGSTOP, &sigIntHandler, nullptr );
		//sigaction( SIGKILL, &sigIntHandler, nullptr );
		//sigaction( SIGTERM, &sigIntHandler, nullptr );
	}
}
#undef var