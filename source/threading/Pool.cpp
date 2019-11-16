#include "stdafx.h"
#include "Pool.h"

namespace Jde::Threading
{
	Pool::Pool( uint threadCount, string_view name ):
		_done(false),
		_index{0},
		_name{ name },
		_joiner(_threads)
	{
		if( threadCount==0 )
			threadCount = std::thread::hardware_concurrency()-2;
		for( uint i=0; i<threadCount; ++i )
			_threads.push_back( std::thread(&Pool::Worker,this) );
	}
	Pool::~Pool()
	{
		//while( !_workQueue.Empty() )
		//	std::this_thread::yield();
		_done = true;
	}

	void Pool::Worker()
	{
		if( _name.size() )
		{
			//auto index = 
			SetThreadDescription( fmt::format("{} {}", _name, _index++) );
		}
		while( !_done || !_workQueue.Empty() )
		{
			std::function<void()> task;
			if( _workQueue.TryPop(task) )
				task();
			else
				std::this_thread::yield();
		}
	}
}