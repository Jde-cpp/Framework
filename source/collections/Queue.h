#pragma once
//adapted from https://livebook.manning.com/#!/book/c-plus-plus-concurrency-in-action-second-edition/chapter-4/v-7/40
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <jde/TypeDefs.h>

#define var const auto
namespace Jde
{
	template<typename T>
	class Queue
	{
	public:
		Queue()=default;
		Queue( const Queue& other );
		void Push(sp<T> new_value)noexcept;
		sp<T> WaitAndPop();
		sp<T> WaitAndPop( Duration duration );
		sp<T> TryPop();
		bool Empty()const noexcept;
		bool ForEach( std::function<void(T&)> func )noexcept;
		uint size()const{ std::lock_guard<std::mutex> lk(_mtx); return _queue.size(); }
	private:
		mutable std::mutex _mtx;
		std::queue<sp<T>> _queue;
		std::condition_variable _cv;
	};
	template<typename T>
	Queue<T>::Queue( const Queue<T>& other )
	{
		std::lock_guard<std::mutex> lk( other._mtx );
		_queue=other._queue;
	}
	template<typename T>
	void Queue<T>::Push( sp<T> new_value )noexcept
	{
		std::lock_guard<std::mutex> lk(_mtx);
		_queue.push(new_value);
		_cv.notify_one();
	}
	template<typename T>
	sp<T> Queue<T>::WaitAndPop()
	{
		std::unique_lock<std::mutex> lk( _mtx );
		_cv.wait_for( lk, Duration::max(), [this]{return !_queue.empty();} );
		auto pItem = _queue.front();
		_queue.pop();
		return pItem;
	}
	template<typename T>
	sp<T> Queue<T>::WaitAndPop( Duration duration )
	{
		std::unique_lock<std::mutex> lk( _mtx );
		bool hasValues = _cv.wait_for( lk, duration, [this]{return !_queue.empty();} );
		sp<T> pItem;
		if( hasValues )
		{
			pItem = _queue.front();
			_queue.pop();
		}
		return pItem;
	}
	template<typename T>
	sp<T> Queue<T>::TryPop()
	{
		std::lock_guard<std::mutex> lk(_mtx);
		const auto empty = _queue.empty();
		auto pItem = empty ? sp<T>{} : _queue.front();
		if( !empty )
			_queue.pop();
		return pItem;
	}
	template<typename T>
	bool Queue<T>::Empty()const noexcept
	{
		std::lock_guard<std::mutex> lk(_mtx);
		return _queue.empty();
	}

	template<typename T>
	bool Queue<T>::ForEach( std::function<void(T&)> func )noexcept
	{
		bool result = false;
		{
			std::lock_guard<std::mutex> lk(_mtx);
			result = !_queue.empty();
			if( result )
			{
				func( *_queue.front() );
				_queue.pop();
			}
		}
		if( result )
			ForEach( func );
		return result;
	}

	template<typename T>
	class QueueValue
	{
	public:
		QueueValue()noexcept;
		QueueValue( const QueueValue& other )noexcept;
		void Push( const T& new_value )noexcept;
		void Push( const vector<T>& new_value )noexcept;
		void WaitAndPop( T& value );
		T WaitAndPop()noexcept;
		optional<T> WaitAndPop( Duration duration )noexcept;
		bool TryPop( T& value )noexcept;
		optional<T> TryPop()noexcept;
		bool Empty()const noexcept;
		bool ForEach( std::function<void(T&)>& func )noexcept;
		uint size()const noexcept{ std::lock_guard<std::mutex> lk(_mtx); return _queue.size(); }
	private:
		mutable std::mutex _mtx;
		std::queue<T> _queue;
		std::condition_variable _cv;
	};
	template<typename T>
	QueueValue<T>::QueueValue()noexcept
	{}

	template<typename T>
	QueueValue<T>::QueueValue( const QueueValue<T>& other )noexcept
	{
		std::lock_guard<std::mutex> lk(other._mtx);
		_queue=other._queue;
	}
	template<typename T>
	void QueueValue<T>::Push( const T& new_value )noexcept
	{
		std::lock_guard<std::mutex> lk(_mtx);
		_queue.push(new_value);
		_cv.notify_one();
	}
	template<typename T>
	void QueueValue<T>::Push( const vector<T>& values )noexcept
	{
		std::lock_guard<std::mutex> lk(_mtx);
		for_each( values.begin(), values.end(), [this](auto x){ _queue.push(x);} );
		_cv.notify_one();
	}
	template<typename T>
	void QueueValue<T>::WaitAndPop( T& value )
	{
		std::unique_lock<std::mutex> lk(_mtx);
		_cv.wait(lk,[this]{return !_queue.empty();});
		value=_queue.front();
		_queue.pop();
	}
	template<typename T>
	T QueueValue<T>::WaitAndPop()noexcept
	{
		std::unique_lock<std::mutex> lk(_mtx);
		_cv.wait( lk, [this]{return !_queue.empty();} );
		auto value = _queue.front();
		_queue.pop();
		return value;
	}
	template<typename T>
	optional<T> QueueValue<T>::WaitAndPop( Duration duration )noexcept
	{
		std::unique_lock<std::mutex> lk( _mtx );
		bool hasValues = _cv.wait_for( lk, duration, [this]{return !_queue.empty();} );
		auto value = hasValues ? _queue.front() : optional<T>{};
		if( hasValues )
			_queue.pop();
		return value;
	}

	template<typename T>
	bool QueueValue<T>::TryPop( T& value )noexcept
	{
		std::lock_guard<std::mutex> lk(_mtx);
		if(_queue.empty())
			return false;
		value=_queue.front();
		_queue.pop();
		return true;
	}
	template<typename T>
	optional<T> QueueValue<T>::TryPop()noexcept
	{
		std::lock_guard<std::mutex> lk( _mtx );
		var empty = _queue.empty();
		var value = empty ? optional<T>{} : _queue.front();
		if( !empty )
			_queue.pop();
		return value;
	}
	template<typename T>
	bool QueueValue<T>::Empty()const noexcept
	{
		std::lock_guard<std::mutex> lk(_mtx);
		return _queue.empty();
	}

	template<typename T>
	bool QueueValue<T>::ForEach( std::function<void(T&)>& func )noexcept
	{
		bool result = false;
		{
			std::lock_guard<std::mutex> lk(_mtx);
			result = !_queue.empty();
			if( result )
			{
				func( _queue.front() );
				_queue.pop();
			}
		}
		if( result )
			ForEach( func );
		return result;
	}
#define SLOCK shared_lock l(_mtx)
#define LOCK unique_lock l(_mtx)
	template<typename T>
	class QueueMove
	{
	public:
		QueueMove()=default;
		QueueMove( T&& initial )noexcept{ _queue.push(move(initial)); }
		void Push( T&& new_value )noexcept;
		optional<T> TryPop( Duration duration )noexcept;
		bool empty()const noexcept{ SLOCK; return _queue.empty(); }
		uint size()const noexcept{ SLOCK; return _queue.size(); }
		optional<T> Pop()noexcept;
		α PopAll()noexcept->vector<T>;
	private:
		mutable std::shared_mutex _mtx;
		std::queue<T> _queue;
		std::condition_variable_any _cv;
	};
	template<typename T>
	void QueueMove<T>::Push( T&& new_value )noexcept
	{
		LOCK;
		_queue.push( std::move(new_value) );
		_cv.notify_one();
	}
	template<typename T>
	optional<T> QueueMove<T>::Pop()noexcept
	{
		LOCK;
		var hasSize = !_queue.empty();
		optional<T> p = hasSize ? optional<T>{ move(_queue.front()) } : std::nullopt;
		if( p )
			_queue.pop();
		return p;
	}

	ⓣ QueueMove<T>::PopAll()noexcept->vector<T>
	{
		LOCK;
		vector<T> results; results.reserve( _queue.size() );
		while( !_queue.empty() )
		{
			results.push_back( move(_queue.front()) );
			_queue.pop();
		}
		return results;
	}

	template<typename T>
	optional<T> QueueMove<T>::TryPop( Duration duration )noexcept
	{
		SLOCK;
		optional<T> result;
		if( _cv.wait_for(l, duration, [this]{return !_queue.empty();}) )
		{
			l.unlock();
			unique_lock l2( _mtx );
			result = std::move( _queue.front() );
			_queue.pop();
		}
		return result;
	}
}
#undef var
#undef SLOCK
#undef LOCK