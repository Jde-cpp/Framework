#include "stdafx.h"


namespace Jde
{
	//static int HandlerRoutine( unsigned long ctrlType );
	BOOL HandlerRoutine( DWORD  ctrlType ) 
	{
		Jde::GetDefaultLogger()->trace( "Caught signal %d", ctrlType );
		for( auto pThread : *_pBackgroundThreads )
			pThread->Interrupt();
		for( auto pThread : *_pBackgroundThreads )
			pThread->Join();
		exit( 1 ); 
//		return TRUE;
	}
	void OSPause()noexcept
	{ 
		INFO( "Pausing main thread. {}", _getpid() );//[*** LOG ERROR ***] [console] [argument index out of range] [2018-04-20 06:47:33]
		std::this_thread::sleep_for(9000h); 
	}
	void AddSignals()
	{
		if( !SetConsoleCtrlHandler(HandlerRoutine, TRUE) )
			THROW( EnvironmentException("Could not set control handler") );
	}
	void AsService()noexcept(false)
	{
		CRITICAL0("AsService not implemented");
		throw "not implemented";
	}

	void Application::Kill( uint /*processId*/ )noexcept
	{
		CRITICAL0( "Kill not implemented" );
	}

	void Application::SetConsoleTitle( string_view title )noexcept{ ::SetConsoleTitle( string(title).c_str() ); }
}
#define getpid _getpid