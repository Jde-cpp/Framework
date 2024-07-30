#pragma once
#ifndef MUTEX_H
#define MUTEX_H
#include <shared_mutex>
#include <boost/core/noncopyable.hpp>
#include "Thread.h"
#include <jde/Assert.h>
#include <jde/log/Log.h>
#include <jde/coroutine/TaskOld.h>
#include "../coroutine/Awaitable.h"

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
	struct CoLock; struct CoGuard;
	class Γ LockAwait : public IAwait{
		using base=IAwait;
	public:
		LockAwait( CoLock& lock )ι:_lock{lock}{}
		α await_ready()ι->bool override;
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override{ return _pGuard ? AwaitResult{move(_pGuard)} : base::await_resume(); }
	private:
		CoLock& _lock;
		up<CoGuard> _pGuard;
	};

	struct Γ CoLock{
		α Lock(){ return LockAwait{*this}; }
	private:
		α TestAndSet()ι->up<CoGuard>;
		α Push( HCoroutine h )ι->up<CoGuard>;
		α Clear()ι->void;

		std::queue<HCoroutine> _queue; mutex _mutex;
		atomic_flag _flag;
		const bool _resumeOnPool{ false };
		friend class LockAwait; friend struct CoGuard;
	};
	struct Γ CoGuard{
		~CoGuard();
	private:
		CoGuard( CoLock& lock )ι;
		CoLock& _lock;
		friend CoLock;
	};
}
namespace Jde::Threading
{
	Φ UniqueLock( str key )ι->std::unique_lock<std::shared_mutex>;


#ifndef NDEBUG //TODORefactor move somewhere else
	struct MyLock
	{
		MyLock( std::mutex& mutex, sv instance, sv name, size_t lineNumber ):
			_pUniqueLock{ nullptr },
			_pUniqueSharedLock{nullptr},
			_pSharedLock{nullptr},
			_description( Jde::format("{}.{} - line={}, thread={}", instance, name, lineNumber, Threading::GetThreadDescription()) )
		{
			TRACE( "unique lock - {}", _description );
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
				TRACE( "shared lock - {}", _description );
				_pSharedLock = make_unique<std::shared_lock<std::shared_mutex>>( mutex );
			}
			else
			{
				TRACE( "unique lock - {}", _description );
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
			_unlocked=true; TRACE( "release - {}", _description );
		}
		~MyLock()
		{
			if( !_unlocked )
				TRACE( "release - {}", _description );
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
		sp<Jde::LogTag> _logTag{ Logging::Tag("mutex") };
	};
#endif
#undef Φ
}
#endif