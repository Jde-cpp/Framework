#include "Thread.h"
#include <algorithm>
#include <atomic>
#include <sstream>

#define var const auto

namespace Jde{
	namespace Threading{
		thread_local uint ThreadId{0};
		static sp<Jde::LogTag> _logTag{ Logging::Tag("threads") };
		α LogTag()ι->sp<Jde::LogTag>{ return _logTag;}
	}
	constexpr uint NameLength = 256;
	thread_local char ThreadName[NameLength]={0};//string shows up as memory leak
	thread_local Threading::HThread AppThreadHandle{0};

	uint Threading::GetAppThreadHandle()ι{ return AppThreadHandle; }
	atomic<Threading::HThread> AppThreadHandleIndex{ (uint)Threading::EThread::AppSpecific };
	Threading::HThread Threading::BumpThreadHandle()ι{ return AppThreadHandleIndex++; }

	void Threading::SetThreadInfo( const ThreadParam& param )ι{
		AppThreadHandle = param.AppHandle;
	}

	void Threading::Run( const size_t iMaxThreadCount, size_t runCount, std::function<void(size_t)> func )ι{
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