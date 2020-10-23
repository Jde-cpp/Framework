#include "Pool.h"

namespace Jde::Threading
{
	Pool::Pool( uint threadCount, string_view name ):
		_done(false),
		_index{0},
		_name{ name },
		_queue{ [](){} },
		_joiner(_threads)
	{
		if( threadCount==0 )
			threadCount = std::thread::hardware_concurrency()-2;
		for( uint i=0; i<threadCount; ++i )
			_threads.push_back( std::thread(&Pool::Worker,this) );
	}
	Pool::~Pool()
	{
		//while( !_queue.Empty() )
		//	std::this_thread::yield();
		_done = true;
	}

	void Pool::Worker()
	{
		if( _name.size() )
		{
			//auto index =
			SetThreadDscrptn( format("{} {}", _name, _index++) );
		}
		while( !_done || !_queue.Empty() )
		{
			std::function<void()> task;
			if( _queue.TryPop(task) )
				task();
			else
				std::this_thread::yield();
		}
	}
}