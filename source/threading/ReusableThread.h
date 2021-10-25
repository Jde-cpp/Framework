#pragma once
#include <condition_variable>
#include <deque>
#include <shared_mutex>
#include <jde/Exception.h>

namespace Jde::Threading
{
	//single thread pool. TODO get rid of.
	template<class TArgs>
	class ReusableThread /*: public std::enable_shared_from_this<ReusableThread<TArgs>>*/
	{
	public:
		ReusableThread( sv name, std::function<void(std::shared_ptr<TArgs>)> method );
		//void NextArgs( std::shared_ptr<TArgs*>& pArgs ){ AddQueue( pArgs ); }
		void Join( std::shared_ptr<TArgs>& pArgs );
		void Join();

	private:
		void AddQueue( std::shared_ptr<TArgs>& pArgs )noexcept;
		void Execute();
		//bool _isFree{true};
		bool _stop{false};
		//bool _runToEnd{false};
		std::function<void( std::shared_ptr<TArgs> )> _method;
		std::string _name;
		std::unique_ptr<std::thread> _pThread{nullptr};
		std::mutex _queueMutex{}; std::condition_variable _queueCV;  //wait for queue to be added to.
		std::mutex _join{}; std::condition_variable _joinCV; //join wait for processing.
		static constexpr size_t _maxSize=1;
		std::deque<std::shared_ptr<TArgs>> _argumentStack; std::shared_mutex _argumentStackMutex;
	};

	template<class TArgs>
	ReusableThread<TArgs>::ReusableThread( sv name, std::function<void(std::shared_ptr<TArgs>)> method ):
		_method( method ),
		_name( name )
	{}

	template<class TArgs>
	void ReusableThread<TArgs>::AddQueue( std::shared_ptr<TArgs>& pArgs )noexcept
	{
		std::unique_lock l( _argumentStackMutex );
		_argumentStack.push_back( pArgs );
		if( !_pThread )
		{
			try
			{
				_pThread = std::make_unique<std::thread>( &ReusableThread::Execute, this );
			}
			catch( const std::bad_weak_ptr& )
			{
				THROW( "This is not a shared pointer." );
			}
		}
		else
			_queueCV.notify_one();
	}

	template<class TArgs>
	void ReusableThread<TArgs>::Join( std::shared_ptr<TArgs>& pArgs )
	{
		std::unique_lock<std::mutex> lk( _join );
		AddQueue( pArgs );
		_joinCV.wait( lk );
	}

	template<class TArgs>
	void ReusableThread<TArgs>::Execute()
	{
		bool cntn = true;
		for( ;cntn; )
		{
			//_isFree = false;
			std::shared_ptr<TArgs> pArguments = nullptr;
			{
				std::unique_lock l( _argumentStackMutex );
				if( _argumentStack.size() )
				{
					pArguments = _argumentStack.front();
					_argumentStack.pop_front();
				}
				if( _argumentStack.size()<_maxSize )
					_joinCV.notify_one();
			}
			if( pArguments )
				_method( pArguments );
			else
			{
				std::unique_lock lk( _queueMutex );
				//_joinCV.notify_one();
				_queueCV.wait( lk );
			}

			cntn = !_stop /*|| _argumentStack.size()*/;
		}
	}
	template<class TArgs>
	void ReusableThread<TArgs>::ReusableThread::Join()
	{
		_stop = true;
		_queueCV.notify_one();
		if( _pThread && _pThread->joinable() )
			_pThread->join();
	}
}