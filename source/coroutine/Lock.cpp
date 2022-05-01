#include "Lock.h"
#include "../threading/Mutex.h"

#define var const auto
namespace Jde
{
	static var& _logLevel{ Logging::TagLevel("locks") };
	flat_map<string,std::deque<std::variant<LockKeyAwait*,coroutine_handle<>>>> _coLocks; std::atomic_flag _coLocksLock;
	α LockWrapperAwait::TryLock( string key, bool /*shared*/ )noexcept->up<CoLockGuard>
	{
		AtomicGuard l( _coLocksLock );
		auto& locks = _coLocks.try_emplace( key ).first->second;
		up<CoLockGuard> y;
		if( locks.size()==0 )
		{
			locks.push_back( nullptr );
			y = mu<CoLockGuard>( move(key), nullptr );
		}
		return y;
	}

	α LockKeyAwait::await_ready()noexcept->bool
	{
		AtomicGuard l( _coLocksLock );
		auto& locks = _coLocks.try_emplace( Key ).first->second;
		locks.push_back( this );
		var ready = locks.size()==1;
		LOG( "({})LockKeyAwait::await_ready={} size={}", Key, ready, locks.size() );
		return locks.size()==1;
	}
	α LockKeyAwait::await_suspend( HCoroutine h )noexcept->void
	{
		Await::AwaitSuspend();
		AtomicGuard l( _coLocksLock ); ASSERT( _coLocks.find(Key)!=_coLocks.end() );
		auto& locks = _coLocks.find( Key )->second;
		if( locks.size()==1 )//if other locks freed after await_ready
		{
			l.unlock();
			h.resume();
		}
		else
		{
			for( uint i=0; !Handle && i<locks.size(); ++i )
			{
				if( locks[i].index()==0 && get<LockKeyAwait*>(locks[i])==this )
					locks[i] = Handle = h;
			}
			ASSERT( Handle );
			LOG( "({})LockKeyAwait::await_suspend size={}", Key, locks.size() );
		}
	}
	α LockKeyAwait::await_resume()noexcept->AwaitResult
	{
		return AwaitResult{ Handle ? mu<CoLockGuard>( Key, Handle ) : mu<CoLockGuard>( Key, this ) };
	}

	CoLockGuard::CoLockGuard( string key, std::variant<LockKeyAwait*,coroutine_handle<>> h )noexcept:
		Handle{h},
		Key{ move(key) }
	{
		LOG( "({})CoLockGuard() index={}", Key, h.index() );
	}
	CoLockGuard::~CoLockGuard()
	{
		AtomicGuard l( _coLocksLock ); ASSERT( _coLocks.find(Key)!=_coLocks.end() && _coLocks.find(Key)->second.size() );
		auto& locks = _coLocks.find( Key )->second; ASSERT( locks[0]==Handle );
		locks.pop_front();
		if( locks.size() )
		{
			if( locks.front().index()==1 )
				CoroutinePool::Resume( move(get<1>(locks.front())) );
			else
				LOG( "({})CoLockGuard - size={}, next is awaitable, should continue.", Key, locks.size() );
		}
		LOG( "~CoLockGuard( {} )", Key );
	}

	α LockWrapperAwait::await_ready()noexcept->bool
	{
		return (bool)(_pLock = TryLock(_key, _shared) );//TODO just hold _coLocksLock wait till key isn't a string.
	}
	α LockWrapperAwait::AwaitSuspend( HCoroutine h )->Task
	{
		_pLock = ( co_await CoLockKey(_key, _shared) ).UP<CoLockGuard>();
		h.resume();
	}
	α LockWrapperAwait::await_suspend( HCoroutine h )noexcept->void
	{
		AwaitSuspend( h );
	}
	α LockWrapperAwait::await_resume()noexcept->AwaitResult
	{
		auto p = _pPromise ? ms<AwaitResult>( move(_pPromise->get_return_object().Result()) ) : ReadyResult();
		if( _f.index()==0 )
			get<0>( _f )( *p );
		else
			get<1>( _f )( *p, move(_pLock) );
		return move( *p );
	}
}