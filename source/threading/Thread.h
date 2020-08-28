#pragma once
#include <string_view>
#include <shared_mutex>

namespace Jde::Threading
{
	void Run( const size_t maxWorkerCount, size_t runCount, std::function<void(size_t)> func );
	//extern thread_local char* ThreadName;
	extern thread_local uint ThreadId;
	JDE_NATIVE_VISIBILITY void SetThreadDescription( std::thread& thread, std::string_view pszDescription );
	JDE_NATIVE_VISIBILITY void SetThreadDescription( const std::string& pszDescription );
	JDE_NATIVE_VISIBILITY const char* GetThreadDescription();

	//taken from https://livebook.manning.com/#!/book/c-plus-plus-concurrency-in-action-second-edition/chapter-8/v-7/1
	class ThreadCollection
	{
	public:
		explicit ThreadCollection( std::vector<std::thread>& threads ):
			_threads( threads)
		{}
		~ThreadCollection()
		{
			for( auto& thread : _threads )
			{
				if( thread.joinable() )
					thread.join();
			}
		}
		private:
			std::vector<std::thread>& _threads;
	};




	//template<typename TMutex>
	class MyLock
	{
	public:
		MyLock( std::mutex& mutex, std::string_view instance, std::string_view name, size_t lineNumber ):
			_pUniqueLock{ nullptr },
			_pUniqueSharedLock{nullptr},
			_pSharedLock{nullptr},
			_description( format("{}.{} - line={}, thread={}", instance, name, lineNumber, Threading::GetThreadDescription()) )
		{
			LOG( _logLevel, "unique lock - {}"sv, _description );
			_pUniqueLock = make_unique<std::unique_lock<std::mutex>>( mutex );
		}
		MyLock( std::shared_mutex& mutex, std::string_view instance, std::string_view name, size_t lineNumber, bool shared = false ):
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
}