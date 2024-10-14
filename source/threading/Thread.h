#pragma once
#include <string_view>
#include <shared_mutex>

namespace Jde{ enum class ELogLevel : int8; }
namespace Jde::Threading
{
	using namespace std::literals;
	typedef uint HThread;
	struct ThreadParam{ string Name; HThread AppHandle; };
//	extern thread_local uint ThreadId;
#define Φ Γ α
	enum class EThread : int
	{
		Application = 0,
		Startup = 1,
		LogServer = 2,
		CoroutinePool = 3,
		AppSpecific = 1 << 10
	};

	Γ uint GetThreadId()ι;
	Γ uint GetAppThreadHandle()ι;
	Γ void SetThreadDscrptn( std::thread& thread, sv description )ι;//TODO move out of threading ns
	Γ void SetThreadDscrptn( sv description )ι;//TODO move out of threading ns
	Γ void SetThreadInfo( const ThreadParam& param )ι;
	α BumpThreadHandle()ι->HThread;
	Φ SetThreadHandle( HThread handle )ι->void;
	Φ GetThreadDescription()ι->const char*;//TODO move out of threading ns & remove Get & abr description
	Φ LogTag()ι->sp<LogTag>;
	void Run( const size_t maxWorkerCount, size_t runCount, std::function<void(size_t)> func )ι;
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
#undef Φ