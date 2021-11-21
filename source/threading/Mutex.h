#pragma once
#include <shared_mutex>
#include <boost/core/noncopyable.hpp>
#include "Thread.h"
#include <jde/Assert.h>
#include <jde/Log.h>
#include <jde/coroutine/Task.h>
#include "../coroutine/Awaitable.h"

#define Φ Γ auto
namespace Jde
{
	enum class ELogLevel : int8;
	struct AtomicGuard final : boost::noncopyable
	{
		AtomicGuard( std::atomic_flag& f )noexcept: _pValue{ &f }
		{
			while( f.test_and_set(std::memory_order_acquire) )
			{
            while( f.test(std::memory_order_relaxed) )
					std::this_thread::yield();
			}
		}
		AtomicGuard( AtomicGuard&& x )noexcept: _pValue{ x._pValue }{ x._pValue = nullptr; }
		~AtomicGuard(){ if( _pValue ){ ASSERT(_pValue->test(std::memory_order_relaxed)) _pValue->clear(std::memory_order_release); } }
		α unlock()noexcept->void{ ASSERT(_pValue) _pValue->clear( std::memory_order_release ); _pValue = nullptr; }
	private:
		atomic_flag* _pValue;
	};
	//Ξ CoLock()noexcept{ return CoLockAwait{move(key)}; }
	struct CoLock; struct CoGuard;
	class Γ LockAwait : public Coroutine::IAwaitable
	{
		using base=Coroutine::IAwaitable;
	public:
		LockAwait( CoLock& lock )noexcept:_lock{lock}{}
		α await_ready()noexcept->bool override;
		α await_suspend( HCoroutine h )noexcept->void override;
		α await_resume()noexcept->TaskResult override{ return _pGuard ? TaskResult{move(_pGuard)} : base::await_resume(); }
	private:
		CoLock& _lock;
		sp<CoGuard> _pGuard;
	};

	struct Γ CoLock
	{
		α Lock(){ return LockAwait{*this}; }
	private:
		α TestAndSet()->sp<CoGuard>;
		α Push( HCoroutine h )->sp<CoGuard>;
		α Clear()->void;

		std::queue<HCoroutine> _queue; mutex _mutex;
		atomic_flag _flag;
		const bool _resumeOnPool{ false };
		//sp<CoGuard> _pGuard;
		friend class LockAwait; friend struct CoGuard;
	};
	struct Γ CoGuard
	{
		~CoGuard();
	private:
		CoGuard( CoLock& lock )noexcept;
		CoLock& _lock;
		friend CoLock;
	};
}
namespace Jde::Threading
{
	Φ UniqueLock( str key )noexcept->std::unique_lock<std::shared_mutex>;

	class Γ LockKeyAwait : public Coroutine::TAwaitable<>
	{
		using base=Coroutine::TAwaitable<>;
	public:
		LockKeyAwait( string key )noexcept:Key{move(key)}{}

		α await_ready()noexcept->bool override;
		α await_suspend( HCoroutine h )noexcept->void override;
		α await_resume()noexcept->TaskResult override;
	private:
		base::THandle Handle{nullptr};
		const string Key;
	};

	struct CoLockGuard final : boost::noncopyable
	{
		CoLockGuard( str Key, std::variant<LockKeyAwait*,coroutine_handle<>> )noexcept;
		~CoLockGuard();
	private:
		std::variant<LockKeyAwait*,coroutine_handle<>> Handle;
		string Key;
	};

	Ξ CoLockKey( string key )noexcept{ return LockKeyAwait{move(key)}; }

#ifndef NDEBUG //TODORefactor move somewhere else
	struct MyLock
	{
		MyLock( std::mutex& mutex, sv instance, sv name, size_t lineNumber ):
			_pUniqueLock{ nullptr },
			_pUniqueSharedLock{nullptr},
			_pSharedLock{nullptr},
			_description( format("{}.{} - line={}, thread={}", instance, name, lineNumber, Threading::GetThreadDescription()) )
		{
			LOG( "unique lock - {}", _description );
			_pUniqueLock = make_unique<std::unique_lock<std::mutex>>( mutex );
		}
		MyLock( std::shared_mutex& mutex, sv instance, sv name, size_t lineNumber, bool shared = false ):
			_pUniqueLock{ nullptr },
			_pUniqueSharedLock{ nullptr },
			_pSharedLock{  nullptr },
			_description( format("{} {}.{} - line={}, thread={}", (shared ? "Shared" : "Unique"), instance, name, lineNumber, Threading::GetThreadDescription()) )
		{
			if( shared )
			{
				LOG( "shared lock - {}", _description );
				_pSharedLock = make_unique<std::shared_lock<std::shared_mutex>>( mutex );
			}
			else
			{
				LOG( "unique lock - {}", _description );
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
			_unlocked=true; LOG( "release - {}", _description );
		}
		~MyLock()
		{
			if( !_unlocked )
				LOG( "release - {}", _description );
		}
		Γ static void SetDefaultLogLevel( ELogLevel logLevel )noexcept;//{ _defaultLogLevel=logLevel; }
		Γ static ELogLevel GetDefaultLogLevel()noexcept;
	private:
		bool _unlocked{false};
		up<std::unique_lock<std::mutex>> _pUniqueLock;
		up<std::unique_lock<std::shared_mutex>> _pUniqueSharedLock;
		up<std::shared_lock<std::shared_mutex>> _pSharedLock;
		std::string _description;
		static ELogLevel _defaultLogLevel;
		const LogTag& _logLevel{ Logging::TagLevel("mutex") };
	};
#endif
#undef Φ
}