
#include "Thread.h"
#include <algorithm>
#include <atomic>
#include <sstream>
#ifdef _MSC_VER
	#include <processthreadsapi.h>
	#include <codecvt>
#endif

#define var const auto
//template<class T> using mup = make_unique<T>::type;

namespace Jde
{

	//thread_local std::string ThreadName;
	constexpr uint NameLength = 255;
	thread_local char ThreadName[NameLength]={0};//string shows up as memory leak
	thread_local char ThreadName2[NameLength]={0};
	thread_local uint Threading::ThreadId{0};
	void Threading::Run( const size_t iMaxThreadCount, size_t runCount, std::function<void(size_t)> func )
	{
		size_t maxThreadCount = std::min( runCount, std::max( iMaxThreadCount,size_t(1)) );
		std::vector<std::thread> threads;
		threads.reserve( maxThreadCount );
		std::atomic<std::size_t> currentIndex{0};
		//std::atomic<std::size_t> theadIndex{0};
		auto threadStart =  [&currentIndex, &runCount, &func]()mutable
		{
			for( size_t index = currentIndex++; index<runCount; index = currentIndex++ )
				func( index );
		};
		for( size_t i=0; i<maxThreadCount; ++i )
			threads.push_back( std::thread(threadStart) );
		for( auto& thread : threads )
			thread.join();
	}

	//https://stackoverflow.com/questions/62243162/how-to-access-setthreaddescription-in-windows-2016-server-version-1607
#ifdef _MSC_VER
	typedef HRESULT (WINAPI *TSetThreadDescription)( HANDLE, PCWSTR );
	TSetThreadDescription pSetThreadDescription = nullptr;

	typedef HRESULT (WINAPI *TGetThreadDescription)( HANDLE, PWSTR* );
	TGetThreadDescription pGetThreadDescription = nullptr;
	 
	void Initialize()noexcept
	{
		HMODULE hKernelBase = GetModuleHandleA("KernelBase.dll");
		if( hKernelBase )
		{
			pSetThreadDescription = reinterpret_cast<TSetThreadDescription>( ::GetProcAddress(hKernelBase, "SetThreadDescription") );
			if( !pSetThreadDescription )
				ERR( "FATAL: failed to get SetThreadDescription() address, error:  {}"sv, ::GetLastError() );
			pGetThreadDescription = reinterpret_cast<TGetThreadDescription>( ::GetProcAddress(hKernelBase, "GetThreadDescription") );
			if( !pGetThreadDescription )
				ERR( "FATAL: failed to get GetThreadDescription() address, error:  {}"sv, ::GetLastError() );
		}
		else
			ERR( "FATAL: failed to get kernel32.dll module handle, error:  {}"sv, ::GetLastError() );
	}
	void WinSetThreadDscrptn( HANDLE h, std::string_view ansiDescription )noexcept
	{
		//std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		//std::wstring wdescription = converter.from_bytes( description );
		std::wstring description; description.reserve( ansiDescription.size() );
		::MultiByteToWideChar( CP_ACP, MB_COMPOSITE, ansiDescription.data(), (int)ansiDescription.size(), description.data(), (int)ansiDescription.size() );
		if( !pSetThreadDescription )
			Initialize();
		if( pSetThreadDescription )
		{
			HRESULT hr = (*pSetThreadDescription)( h, description.c_str() );
			if( FAILED(hr) )
				WARN( "Could not set name for thread({}) {} - ({}) - {} "sv, h, ansiDescription, hr, ::GetLastError() );
		}
	}
#endif

		const char* Threading::GetThreadDescription()noexcept
		{
			if( std::strlen(ThreadName)==0 )
			{
#ifdef _MSC_VER
				PWSTR pszThreadDescription;
				HANDLE threadId = 0;
				threadId = reinterpret_cast<HANDLE>( (UINT_PTR)::GetCurrentThreadId() );
				if( !pGetThreadDescription )
					Initialize();
				if( !pGetThreadDescription )
					return ThreadName;
				var hr = (*pGetThreadDescription)( threadId, &pszThreadDescription );
				if( SUCCEEDED(hr) )
				{
					var size = wcslen(pszThreadDescription);
					auto pDescription = new char[ size ];
					uint size2;
					wcstombs_s( &size2, pDescription, size, pszThreadDescription, size );
					::LocalFree( pszThreadDescription );
					std::strncpy( ThreadName, pDescription, sizeof(ThreadName)/sizeof(ThreadName[0]) );
					//ThreadName = pDescription;
					delete[] pDescription;
				}
#else
				ThreadId = pthread_self();
				//char thread_name[NameLength];
				var rc = pthread_getname_np( ThreadId, ThreadName, NameLength );
    			if (rc != 0)
        			ERR( "pthread_getname_np returned {}"sv, rc );
				//else
					//cscpy(ThreadName = thread_name;
#endif
			}
			return ThreadName;
		}

	void Threading::SetThreadDscrptn( std::thread& thread, std::string_view pszDescription )noexcept
	{
#ifdef _MSC_VER
		WinSetThreadDscrptn( static_cast<HANDLE>(thread.native_handle()), pszDescription );
#else
	   pthread_setname_np( thread.native_handle(), string(pszDescription).c_str() );
#endif
	}
#ifndef _MSC_VER
	#include <sys/prctl.h>
#endif
	void Threading::SetThreadDscrptn( const std::string& description )noexcept
	{
#ifdef _MSC_VER
		WinSetThreadDscrptn( ::GetCurrentThread(), description );
#else
		strncpy( ThreadName, description.c_str(), NameLength );
//		strncpy( ThreadName2, (string("y")+string(description)).c_str(), 15 );
		prctl( PR_SET_NAME, ThreadName, 0, 0, 0 );
		ThreadId = pthread_self();
#endif
	}

	ELogLevel Threading::MyLock::_defaultLogLevel{ ELogLevel::Trace };
	void Threading::MyLock::SetDefaultLogLevel( ELogLevel logLevel )noexcept{ _defaultLogLevel=logLevel; }
	ELogLevel Threading::MyLock::GetDefaultLogLevel()noexcept{ return _defaultLogLevel; }


}