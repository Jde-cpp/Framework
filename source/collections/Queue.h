#pragma once
//adapted from https://livebook.manning.com/#!/book/c-plus-plus-concurrency-in-action-second-edition/chapter-4/v-7/40
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace Jde
{
	template<typename T>
	class Queue
	{
	public:
		Queue()=default;
		//Queue( function<void()> onPush );
		Queue( const Queue& other );
		void Push(sp<T> new_value)noexcept;
		std::shared_ptr<T> WaitAndPop();
		std::shared_ptr<T> WaitAndPop( Duration duration );
		std::shared_ptr<T> TryPop();
		bool Empty()const noexcept;
		bool ForEach( std::function<void(T&)>& func );
		uint size()const{ std::lock_guard<std::mutex> lk(_mtx); return _queue.size(); }
	private:
		//function<void()> _onPush;
		mutable std::mutex _mtx;
		std::queue<sp<T>> _queue;
		std::condition_variable _cv;
	};
/*	template<typename T>
	Queue<T>::Queue( function<void()> onPush )noexcept:
		_onPush{ onPush }
	{}*/
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
//		_onPush();
	}
	template<typename T>
	std::shared_ptr<T> Queue<T>::WaitAndPop()
	{
		std::unique_lock<std::mutex> lk( _mtx );
		_cv.wait( lk, [this]{return !_queue.empty();} );
		auto pItem = _queue.front();
		_queue.pop();
		return pItem;
	}
	template<typename T>
	std::shared_ptr<T> Queue<T>::WaitAndPop( Duration duration )
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
	std::shared_ptr<T> Queue<T>::TryPop()
	{
		std::lock_guard<std::mutex> lk(_mtx);
		const auto empty = _queue.empty();
		auto pItem = empty ? std::shared_ptr<T>{} : _queue.front();
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
	bool Queue<T>::ForEach( std::function<void(T&)>& func )
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
		QueueValue()=default;
		QueueValue( const QueueValue& other )noexcept;
		void Push( const T& new_value )noexcept;
		void WaitAndPop( T& value );
		std::shared_ptr<T> WaitAndPop()noexcept;
		bool TryPop( T& value )noexcept;
		std::shared_ptr<T> TryPop()noexcept;
		bool Empty()const noexcept;
		bool ForEach( std::function<void(T&)>& func )noexcept;
		uint size()const noexcept{ std::lock_guard<std::mutex> lk(_mtx); return _queue.size(); }
	private:
		mutable std::mutex _mtx;
		std::queue<T> _queue;
		std::condition_variable _cv;
	};

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
	void QueueValue<T>::WaitAndPop( T& value )
	{
		std::unique_lock<std::mutex> lk(_mtx);
		_cv.wait(lk,[this]{return !_queue.empty();});
		value=_queue.front();
		_queue.pop();
	}
	template<typename T>
	std::shared_ptr<T> QueueValue<T>::WaitAndPop()noexcept
	{
		std::unique_lock<std::mutex> lk(_mtx);
		_cv.wait(lk,[this]{return !_queue.empty();});
		std::shared_ptr<T> res(std::make_shared<T>(_queue.front()));
		_queue.pop();
		return res;
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
	std::shared_ptr<T> QueueValue<T>::TryPop()noexcept
	{
		std::lock_guard<std::mutex> lk(_mtx);
		if(_queue.empty())
			return std::shared_ptr<T>();
		std::shared_ptr<T> res(std::make_shared<T>(_queue.front()));
		_queue.pop();
		return res;
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
}