#pragma once
#include <string_view>
#include <shared_mutex>
#include <jde/Log.h>

namespace Jde{ enum class ELogLevel : uint8; }
namespace Jde::Threading
{
	using namespace std::literals;
	typedef uint HThread;
	struct ThreadParam{ string Name; HThread AppHandle; };
	extern thread_local uint ThreadId;

	enum class EThread : int
	{
		Application = 0,
		Startup = 1,
		LogServer = 2,
		CoroutinePool = 3,

		AppSpecific = 1 << 10

	};

	JDE_NATIVE_VISIBILITY uint GetThreadId()noexcept;
	JDE_NATIVE_VISIBILITY uint GetAppThreadHandle()noexcept;
	JDE_NATIVE_VISIBILITY void SetThreadDscrptn( std::thread& thread, sv description )noexcept;
	JDE_NATIVE_VISIBILITY void SetThreadDscrptn( const std::string& description )noexcept;
	JDE_NATIVE_VISIBILITY void SetThreadInfo( const ThreadParam& param )noexcept;
	HThread BumpThreadHandle()noexcept;
	//JDE_NATIVE_VISIBILITY void SetThreadInfo( ThreadParam& param )noexcept;
	JDE_NATIVE_VISIBILITY const char* GetThreadDescription()noexcept;

	void Run( const size_t maxWorkerCount, size_t runCount, std::function<void(size_t)> func )noexcept;
	//taken from https://livebook.manning.com/#!/book/c-plus-plus-concurrency-in-action-second-edition/chapter-8/v-7/1
	class ThreadCollection //TODO refactor [re]move this
	{
	public:
		explicit ThreadCollection( std::vector<std::thread>& threads ):
			_threads( threads)
		{}
		~ThreadCollection()
		{
			for( auto& thread : _threads )
			{
				if( thread.joinable() )
					thread.join();
			}
		}
		private:
			std::vector<std::thread>& _threads;
	};
}