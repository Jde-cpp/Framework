
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

namespace Jde::Threading
{

	//thread_local std::string ThreadName;
	constexpr uint NameLength = 255;
	thread_local char ThreadName[NameLength]={0};//string shows up as memory leak
	thread_local char ThreadName2[NameLength]={0};
	thread_local uint ThreadId{0};
	void Run( const size_t iMaxThreadCount, size_t runCount, std::function<void(size_t)> func )
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

	const char* GetThreadDescription()
	{
		if( std::strlen(ThreadName)==0 )
		{
#ifdef _MSC_VER
			PWSTR pszThreadDescription;
			HANDLE threadId = 0;
			threadId = reinterpret_cast<HANDLE>( (UINT_PTR)::GetCurrentThreadId() );
			var hr = ::GetThreadDescription( threadId, &pszThreadDescription );
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


	void SetThreadDescription( std::thread& thread, std::string_view pszDescription )
	{
#ifdef _WINDOWS
		//DWORD threadId = ::GetThreadId( static_cast<HANDLE>( thread.native_handle() ) );
		std::wstring description; description.reserve( pszDescription.size() );
		MultiByteToWideChar( CP_ACP, MB_COMPOSITE, pszDescription.data(), (int)pszDescription.size(), description.data(), (int)pszDescription.size() );
		auto handle = static_cast<HANDLE>( thread.native_handle() );
		HRESULT hr = ::SetThreadDescription( handle, description.c_str() );
		if( FAILED(hr) )
			GetDefaultLogger()->warn( "Could not set name for thread({}) {}", handle, pszDescription );
#else
	   pthread_setname_np( thread.native_handle(), string(pszDescription).c_str() );
#endif
	}
#ifndef _WINDOWS
	#include <sys/prctl.h>
#endif
	void SetThreadDescription( const std::string& description )
	{
#ifdef _WINDOWS
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		//std::string narrow = converter.to_bytes(wide_utf16_source_string);
		std::wstring wdescription = converter.from_bytes( description );
		//std::wstring wdescription; wdescription.reserve( description.size() );
		//MultiByteToWideChar( CP_ACP, MB_COMPOSITE, description.data(), (int)description.size(), wdescription.data(), (int)description.size() );
		HRESULT hr = ::SetThreadDescription( ::GetCurrentThread(), wdescription.c_str() );
		if( FAILED(hr) )
			WARN( "Could not set name for thread {}", string(description).c_str() );
#else
		strncpy( ThreadName, description.c_str(), NameLength );
//		strncpy( ThreadName2, (string("y")+string(description)).c_str(), 15 );
		prctl( PR_SET_NAME, ThreadName, 0, 0, 0 );
		ThreadId = pthread_self();
#endif
	}

	ELogLevel MyLock::_defaultLogLevel{ ELogLevel::Trace };
	void MyLock::SetDefaultLogLevel( ELogLevel logLevel )noexcept{ _defaultLogLevel=logLevel; }
	ELogLevel MyLock::GetDefaultLogLevel()noexcept{ return _defaultLogLevel; }


}