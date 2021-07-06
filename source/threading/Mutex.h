#pragma once
#include <shared_mutex>
#include <boost/core/noncopyable.hpp>
#include "Thread.h"
#include <jde/Log.h>
#include <jde/coroutine/Task.h>
#include "../coroutine/Awaitable.h"

#define 🚪 JDE_NATIVE_VISIBILITY auto
namespace Jde{ enum class ELogLevel : uint8; }
namespace Jde::Threading
{
	🚪 UniqueLock( str key )noexcept->std::unique_lock<std::shared_mutex>;

	struct JDE_NATIVE_VISIBILITY AtomicGuard final : boost::noncopyable
	{
		AtomicGuard( atomic<bool>& v )noexcept: Value{ v }
		{
			while( Value.exchange(true) )
				std::this_thread::yield();
		}
		~AtomicGuard(){ ASSERT( Value ); Value = false; }
		atomic<bool>& Value;
	};

	struct JDE_NATIVE_VISIBILITY CoLockAwatiable : Coroutine::TAwaitable<>
	{
		CoLockAwatiable( str key )noexcept:Key{key}{}
		using base=Coroutine::TAwaitable<>;
		bool await_ready()noexcept override;
		void await_suspend( typename base::THandle h )noexcept override;
		typename base::TResult await_resume()noexcept override;
	private:
		base::THandle Handle{nullptr};
		const string Key;
	};

	struct CoLockGuard final : boost::noncopyable
	{
		CoLockGuard( str Key, std::variant<CoLockAwatiable*,coroutine_handle<>> )noexcept;
		~CoLockGuard();
	private:
		std::variant<CoLockAwatiable*,coroutine_handle<>> Handle;
		string Key;
	};

	inline α CoLock( str key )noexcept{ return CoLockAwatiable{key}; }


#ifndef NDEBUG //TODORefactor move somewhere else
	struct MyLock
	{
		MyLock( std::mutex& mutex, sv instance, sv name, size_t lineNumber ):
			_pUniqueLock{ nullptr },
			_pUniqueSharedLock{nullptr},
			_pSharedLock{nullptr},
			_description( format("{}.{} - line={}, thread={}", instance, name, lineNumber, Threading::GetThreadDescription()) )
		{
			LOG( _logLevel, "unique lock - {}"sv, _description );
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
				LOG( _logLevel, "shared lock - {}"sv, _description );
				_pSharedLock = make_unique<std::shared_lock<std::shared_mutex>>( mutex );
			}
			else
			{
				LOG( _logLevel, "unique lock - {}"sv, _description );
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
			_unlocked=true; LOG( _logLevel, "release - {}"sv, _description );
		}
		~MyLock()
		{
			if( !_unlocked )
				LOG( _logLevel, "release - {}"sv, _description );
		}
		JDE_NATIVE_VISIBILITY static void SetDefaultLogLevel( ELogLevel logLevel )noexcept;//{ _defaultLogLevel=logLevel; }
		JDE_NATIVE_VISIBILITY static ELogLevel GetDefaultLogLevel()noexcept;
	private:
		bool _unlocked{false};
		std::unique_ptr<std::unique_lock<std::mutex>> _pUniqueLock;
		std::unique_ptr<std::unique_lock<std::shared_mutex>> _pUniqueSharedLock;
		std::unique_ptr<std::shared_lock<std::shared_mutex>> _pSharedLock;
		std::string _description;
		static ELogLevel _defaultLogLevel;
		ELogLevel _logLevel{ GetDefaultLogLevel() };
	};
#endif
#undef 🚪
}