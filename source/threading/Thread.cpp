#include "Thread.h"
#include <algorithm>
#include <atomic>
#include <sstream>

#define var const auto

namespace Jde
{
	constexpr uint NameLength = 255;
	thread_local char ThreadName[NameLength]={0};//string shows up as memory leak
	thread_local uint Threading::ThreadId{0};
	thread_local Threading::HThread AppThreadHandle{0};

	uint Threading::GetAppThreadHandle()noexcept{ return AppThreadHandle; }
	atomic<Threading::HThread> AppThreadHandleIndex{ (uint)Threading::EThread::AppSpecific };
	Threading::HThread Threading::BumpThreadHandle()noexcept{ return AppThreadHandleIndex++; }

	void Threading::SetThreadInfo( const ThreadParam& param )noexcept
	{
		AppThreadHandle = param.AppHandle;
		SetThreadDscrptn( param.Name );
	}

	void Threading::Run( const size_t iMaxThreadCount, size_t runCount, std::function<void(size_t)> func )noexcept
	{
		size_t maxThreadCount = std::min( runCount, std::max( iMaxThreadCount,size_t(1)) );
		std::vector<std::thread> threads;
		threads.reserve( maxThreadCount );
		std::atomic<std::size_t> currentIndex{0};
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
}