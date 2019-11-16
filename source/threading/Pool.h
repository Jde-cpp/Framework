
#pragma once
#include <condition_variable>
#include <deque>
#include <shared_mutex>
#include "Thread.h"
#include "../collections/Queue.h"
//#include "../Exception.h"

namespace Jde::Threading
{
	struct JDE_NATIVE_VISIBILITY Pool
	{
		Pool( uint threadCount=0, string_view name="Pool" );
		~Pool();

    	template<typename FunctionType>
    	void Submit( FunctionType f );
	private:
		std::atomic_bool _done;
		std::atomic<uint> _index;
		const string _name;
    	QueueValue<std::function<void()>> _workQueue;
    	std::vector<std::thread> _threads;
    	ThreadCollection _joiner;
		void Worker();
	};

	template<typename FunctionType>
	void Pool::Submit( FunctionType f )
	{
		_workQueue.Push( std::function<void()>(f) );
	}
}