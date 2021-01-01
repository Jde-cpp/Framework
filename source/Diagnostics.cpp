/*
#include "Diagnostics.h"
#ifdef _MSC_VER
//	#include <Windows.h>
	#include <Psapi.h>
	//#include <WinSock2.h>
#else
	#include <errno.h>
#endif


namespace Jde
{
	size_t Diagnostics::GetMemorySize()
	{
#ifdef _MSC_VER
		PROCESS_MEMORY_COUNTERS memCounter;
		/*BOOL result =* / GetProcessMemoryInfo( ::GetCurrentProcess(), &memCounter, sizeof( memCounter ) );
		return memCounter.WorkingSetSize;
#else
//https://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-runtime-using-c
		uint size = 0;
		FILE* fp = fopen( "/proc/self/statm", "r" );
		if( fp!=nullptr )
		{
			long rss = 0L;
			if( fscanf( fp, "%*s%ld", &rss ) == 1 )
				size = (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);
			fclose( fp );
		}
		return size;
#endif
	}
	fs::path Diagnostics::ApplicationName()
	{
#ifdef _WINDOWS
		char* szExeFileName[MAX_PATH]; 
		::GetModuleFileNameA( NULL, (char*)szExeFileName, MAX_PATH );
		return fs::path( (char*)szExeFileName );
#else
		return std::filesystem::canonical( "/proc/self/exe" ).parent_path(); 
		return fs::path( program_invocation_name );
#endif
	}

	string Diagnostics::HostName()
	{
#if _MSC_VER
		constexpr uint maxHostName = 1024;
#else
		constexpr uint maxHostName = HOST_NAME_MAX;
#endif
		char hostname[maxHostName];
		//gethostname( hostname, maxHostName );
		return hostname;
	}
	
	uint Diagnostics::ProcessId()
	{
#if _MSC_VER
			return _getpid();
#else
			return getpid();
#endif
	}
}
*/