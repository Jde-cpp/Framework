#pragma once
#ifndef MUTEX_H
#define MUTEX_H
#include <shared_mutex>
#include <queue>
#include <boost/core/noncopyable.hpp>
#include "Thread.h"

#include <jde/framework/coroutine/Await.h>

#define Φ Γ auto
namespace Jde{
	enum class ELogLevel : int8;
	struct AtomicGuard final : boost::noncopyable{
		AtomicGuard( std::atomic_flag& f )ι: _pValue{ &f }{
			while( f.test_and_set(std::memory_order_acquire) ){
        while( f.test(std::memory_order_relaxed) )
					std::this_thread::yield();
			}
		}
		AtomicGuard( AtomicGuard&& x )ι: _pValue{ x._pValue }{ x._pValue = nullptr; }
		~AtomicGuard(){ if( _pValue ){ ASSERT(_pValue->test(std::memory_order_relaxed)) _pValue->clear(std::memory_order_release); } }
		α unlock()ι->void{ ASSERT(_pValue) _pValue->clear( std::memory_order_release ); _pValue = nullptr; }
	private:
		atomic_flag* _pValue;
	};

	struct CoLock;
	struct Γ CoGuard{
		CoGuard()ι{ ASSERT(false); }
		~CoGuard();
		α unlock()ι->void;
	private:
		CoGuard( CoLock& lock )ι;
		CoLock* _lock{};
		friend CoLock;
	};

	class Γ LockAwait : public TAwait<CoGuard>{
		using base=TAwait<CoGuard>;
	public:
		LockAwait( CoLock& lock )ι;
		~LockAwait();
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ι->CoGuard override;
	private:
		CoLock& _lock;
		optional<CoGuard> _pGuard;
	};

	struct Γ CoLock{
		α Lock(){ return LockAwait{*this}; }
	private:
		α TestAndSet()ι->optional<CoGuard>;
		α Push( LockAwait::Handle h )ι->optional<CoGuard>;
		α Clear()ι->void;

		std::queue<LockAwait::Handle> _queue; mutex _mutex;
		atomic_flag _flag;
		friend class LockAwait; friend struct CoGuard;
	};
}
namespace Jde::Threading
{
	Φ UniqueLock( str key )ι->std::unique_lock<std::shared_mutex>;


#ifndef NDEBUG //TODORefactor move somewhere else
	struct MyLock{
		MyLock( std::mutex& mutex, sv instance, sv name, size_t lineNumber ):
			_pUniqueLock{ nullptr },
			_pUniqueSharedLock{nullptr},
			_pSharedLock{nullptr},
			_description( Jde::format("{}.{} - line={}, thread={}", instance, name, lineNumber, Threading::GetThreadDescription()) )
		{
			Trace( ELogTags::Locks, "unique lock - {}", _description );
			_pUniqueLock = make_unique<std::unique_lock<std::mutex>>( mutex );
		}
		MyLock( std::shared_mutex& mutex, sv instance, sv name, size_t lineNumber, bool shared = false ):
			_pUniqueLock{ nullptr },
			_pUniqueSharedLock{ nullptr },
			_pSharedLock{  nullptr },
			_description( Jde::format("{} {}.{} - line={}, thread={}", (shared ? "Shared" : "Unique"), instance, name, lineNumber, Threading::GetThreadDescription()) )
		{
			if( shared )
			{
				Trace( ELogTags::Locks, "shared lock - {}", _description );
				_pSharedLock = make_unique<std::shared_lock<std::shared_mutex>>( mutex );
			}
			else
			{
				Trace( ELogTags::Locks, "unique lock - {}", _description );
				_pUniqueSharedLock = make_unique<std::unique_lock<std::shared_mutex>>( mutex );
			}
		}
		void unlock()
		{
			if( _pUniqueLock )
				_pUniqueLock->unlock();
			else if( _pUniqueSharedLock )
				_pUniqueSharedLock->unlock();
			else if( _pSharedLock )
				_pSharedLock->unlock();
			_unlocked=true; Trace( ELogTags::Locks, "release - {}", _description );
		}
		~MyLock()
		{
			if( !_unlocked )
				Trace( ELogTags::Locks, "release - {}", _description );
		}
		Γ static void SetDefaultLogLevel( ELogLevel logLevel )ι;//{ _defaultLogLevel=logLevel; }
		Γ static ELogLevel GetDefaultLogLevel()ι;
	private:
		bool _unlocked{false};
		up<std::unique_lock<std::mutex>> _pUniqueLock;
		up<std::unique_lock<std::shared_mutex>> _pUniqueSharedLock;
		up<std::shared_lock<std::shared_mutex>> _pSharedLock;
		std::string _description;
		static ELogLevel _defaultLogLevel;
	};
#endif
#undef Φ
}
#endif