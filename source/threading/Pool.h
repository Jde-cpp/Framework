
#pragma once
#include <condition_variable>
#include <deque>
#include <shared_mutex>
#include "Thread.h"
#include "../collections/Queue.h"
#include <jde/framework/process.h>
#include "InterruptibleThread.h"

namespace Jde::Threading{
	struct Γ Pool	{
		Pool( uint threadCount=0, sv name="Pool" );
		~Pool();

    	template<typename FunctionType>
    	void Submit( FunctionType f );
	private:
		std::atomic_bool _done;
		std::atomic<uint> _index;
		const string _name;
    	QueueValue<std::function<void()>> _queue;
    	std::vector<std::thread> _threads;
    	ThreadCollection _joiner;
		void Worker();
	};

	template<typename FunctionType>
	void Pool::Submit( FunctionType f )
	{
		_queue.Push( std::function<void()>(f) );
	}
#ifdef Unused
	template<typename T>
	struct TypePool : IShutdown{
		TypePool( uint8 threadCount, sv name )ι;
		virtual ~TypePool(){ TRACE("~TypePool"sv); }

		virtual void Execute( sp<T> pValue )ι=0;
		void Push( const sp<T> pValue )ι;
		void Shutdown()ι override;

	private:
		void Run( string name )ι;
		void AddThread()ι;

		const uint8 MaxThreads;
		const string Name;
		std::atomic<uint8> RunningCount{0};
    	std::vector<sp<InterruptibleThread>> _threads; mutable std::mutex _mtx;
		Queue<T> _queue;
	};
#define let const auto
	template<typename T>
	TypePool<T>::TypePool( uint8 threadCount, sv name )ι:
		MaxThreads{ threadCount },
		Name{name}
	{
		//AddThread();
	}

	template<typename T>
	void TypePool<T>::AddThread()ι
	{
		unique_lock l{_mtx};
		if( RunningCount<MaxThreads )
		{
			let name = Jde::format( "{}-{}", Name, _threads.size() );
			auto pThread = make_shared<InterruptibleThread>( name, [&, name](){Run(name);} );
			_threads.push_back( pThread );
		}
	}

	template<typename T>
	void TypePool<T>::Run( string name )ι
	{
		TRACE( "({}) Starting"sv, name  );
		while( !GetThreadInterruptFlag().IsSet() )//fyi existing _queue is thrown out.
		{
			unique_lock l{ _mtx };
			sp<T> p = _queue.TryPop();
			if( p )
			{
				++RunningCount;
				l.unlock();
				Execute( p );
				l.lock();
				--RunningCount;
			}
			else
			{
				_threads.erase( remove_if(_threads.begin(),_threads.end(), [](auto pThread){ return pThread->IsDone(); }), _threads.end() );
				break;
			}
		}
		TRACE( "({}) Leaving"sv, name  );
	}

	template<typename T>
	void TypePool<T>::Push( const sp<T> pValue )ι
	{
		_queue.Push( pValue );
		AddThread();
	}
	template<typename T>
	void TypePool<T>::Shutdown()ι
	{
		TRACE( "TypePool<T>::Shutdown"sv );
		unique_lock l{_mtx};
		while( _threads.size() )
		{
			auto pThread = _threads[_threads.size()-1];
			l.unlock();
			pThread->Interrupt();
			pThread->Join();
			l.lock();
			_threads.pop_back();
		}
	}
#endif
}
#undef let
#undef _logTag
