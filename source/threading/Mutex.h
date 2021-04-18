#pragma once
#include <string_view>
#include <shared_mutex>
#include "Thread.h"
#include "../log/Logging.h"

namespace Jde{ enum class ELogLevel : uint8; }
namespace Jde::Threading
{
	JDE_NATIVE_VISIBILITY std::unique_lock<std::shared_mutex> UniqueLock( str key )noexcept;
	//std::unique_ptr<std::unique_lock<std::shared_mutex>> UniqueLock( str key )noexcept;
	//std::shared_lock<std::shared_mutex> SharedLock( str key )noexcept;

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
}