#include "Mutex.h"
#include <boost/container/flat_map.hpp>
#define var const auto
namespace Jde
{
	static const LogTag& _logLevel = Logging::TagLevel( "locks" );

	CoGuard::CoGuard( CoLock& lock )noexcept:
		_lock{lock}
	{
		LOG( "CoGuard" );
	}

	CoGuard::~CoGuard()
	{
		LOG( "~CoGuard" );
		_lock.Clear();
	}

	α LockAwait::await_ready()noexcept->bool
	{
		_pGuard = _lock.TestAndSet();
		return !!_pGuard;
	}
	α LockAwait::await_suspend( HCoroutine h )noexcept->void
	{
		base::await_suspend( h );
		if( (_pGuard = _lock.Push(h)) )
			h.resume();
	}
	α CoLock::TestAndSet()->sp<CoGuard>
	{
		lock_guard l{ _mutex };
		return _flag.test_and_set(std::memory_order_acquire) ? sp<CoGuard>{} : sp<CoGuard>( new CoGuard(*this) );
	}
	α CoLock::Push( HCoroutine h )->sp<CoGuard>
	{
		lock_guard l{ _mutex };
		var p = _flag.test_and_set(std::memory_order_acquire) ? sp<CoGuard>{} : sp<CoGuard>{ new CoGuard(*this) };
		if( !p )
			_queue.push( move(h) );
		return p;
	}
	α CoLock::Clear()->void
	{
		lock_guard l{ _mutex };
		if( !_queue.empty() )
		{
			auto h = _queue.front();
			_queue.pop();
			h.promise().get_return_object().SetResult( sp<CoGuard>{ new CoGuard(*this) } );
			if( _resumeOnPool )
				CoroutinePool::Resume( move(h) );
			else
				h.resume();
		}
		else
			_flag.clear();
	}
}
namespace Jde::Threading
{
	flat_map<string,std::deque<std::variant<LockKeyAwait*,coroutine_handle<>>>> _coLocks; std::atomic_flag _coLocksLock;

	CoLockGuard::CoLockGuard( str key, std::variant<LockKeyAwait*,coroutine_handle<>> h )noexcept:
		Handle{h},
		Key{key}
	{}
	CoLockGuard::~CoLockGuard()
	{
		AtomicGuard l( _coLocksLock ); ASSERT( _coLocks.find(Key)!=_coLocks.end() && _coLocks.find(Key)->second.size() );
		auto& locks = _coLocks.find( Key )->second; ASSERT( locks[0]==Handle );
		locks.pop_front();
		if( locks.size() )
		{
			ASSERT( locks.front().index()==1 );
			Coroutine::CoroutinePool::Resume( move(get<1>(locks.front())) );
		}
	}

	bool LockKeyAwait::await_ready()noexcept
	{
		AtomicGuard l( _coLocksLock );
		auto& locks = _coLocks.try_emplace( Key ).first->second;
		locks.push_back( this );
		return locks.size()==1;
	}
	void LockKeyAwait::await_suspend( typename base::THandle h )noexcept
	{
		base::await_suspend( h );
		AtomicGuard l( _coLocksLock ); ASSERT( _coLocks.find(Key)!=_coLocks.end() );
		auto& locks = _coLocks.find( Key )->second;
		for( uint i=0; !Handle && i<locks.size(); ++i )
		{
			if( locks[i].index()==0 && get<LockKeyAwait*>(locks[i])==this )
				locks[i] = Handle = h;
 		}
		ASSERT( Handle );
	}
	LockKeyAwait::base::TResult LockKeyAwait::await_resume()noexcept
	{
		return Coroutine::TaskResult{ Handle ? make_shared<CoLockGuard>( Key, Handle ) : make_shared<CoLockGuard>( Key, this ) };
	}

#ifndef NDEBUG
	ELogLevel Threading::MyLock::_defaultLogLevel{ ELogLevel::Trace };
	void Threading::MyLock::SetDefaultLogLevel( ELogLevel logLevel )noexcept{ _defaultLogLevel=logLevel; }
	ELogLevel Threading::MyLock::GetDefaultLogLevel()noexcept{ return _defaultLogLevel; }
#endif
	static boost::container::flat_map<string,sp<std::shared_mutex>> _mutexes;
	std::mutex _mutex;
	std::unique_lock<std::shared_mutex> UniqueLock( str key )noexcept
	{
		unique_lock l{_mutex};

		auto p = _mutexes.find( key );
		auto pKeyMutex = p == _mutexes.end() ? _mutexes.emplace( key, make_shared<std::shared_mutex>() ).first->second : p->second;
		for( auto pExisting = _mutexes.begin(); pExisting != _mutexes.end();  )
			pExisting = pExisting->first!=key && pExisting->second.use_count()==1 && pExisting->second->try_lock() ? _mutexes.erase( pExisting ) : std::next( pExisting );
		l.unlock();
		return unique_lock{ *pKeyMutex };
	}
}