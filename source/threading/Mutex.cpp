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
	α CoLock::TestAndSet()->up<CoGuard>
	{
		lock_guard l{ _mutex };
		return _flag.test_and_set(std::memory_order_acquire) ? up<CoGuard>{} : up<CoGuard>( new CoGuard(*this) );
	}
	α CoLock::Push( HCoroutine h )->up<CoGuard>
	{
		lock_guard l{ _mutex };
		auto p = _flag.test_and_set(std::memory_order_acquire) ? up<CoGuard>{} : up<CoGuard>{ new CoGuard(*this) };
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
			h.promise().get_return_object().SetResult( up<CoGuard>{ new CoGuard(*this) } );
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
#ifndef NDEBUG
	ELogLevel Threading::MyLock::_defaultLogLevel{ ELogLevel::Trace };
	void Threading::MyLock::SetDefaultLogLevel( ELogLevel logLevel )noexcept{ _defaultLogLevel=logLevel; }
	ELogLevel Threading::MyLock::GetDefaultLogLevel()noexcept{ return _defaultLogLevel; }
#endif
	static boost::container::flat_map<string,sp<shared_mutex>> _mutexes;
	mutex _mutex;
	unique_lock<shared_mutex> UniqueLock( str key )noexcept
	{
		unique_lock l{_mutex};

		auto p = _mutexes.find( key );
		auto pKeyMutex = p == _mutexes.end() ? _mutexes.emplace( key, ms<shared_mutex>() ).first->second : p->second;
		for( auto pExisting = _mutexes.begin(); pExisting != _mutexes.end();  )
			pExisting = pExisting->first!=key && pExisting->second.use_count()==1 && pExisting->second->try_lock() ? _mutexes.erase( pExisting ) : std::next( pExisting );
		l.unlock();
		return unique_lock{ *pKeyMutex };
	}
}