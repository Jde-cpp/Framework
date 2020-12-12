#include "Mutex.h"
#include <boost/container/flat_map.hpp>
namespace Jde{ enum class ELogLevel : uint8; }
namespace Jde::Threading
{
	ELogLevel Threading::MyLock::_defaultLogLevel{ ELogLevel::Trace };
	void Threading::MyLock::SetDefaultLogLevel( ELogLevel logLevel )noexcept{ _defaultLogLevel=logLevel; }
	ELogLevel Threading::MyLock::GetDefaultLogLevel()noexcept{ return _defaultLogLevel; }

	static boost::container::flat_map<string,sp<std::shared_mutex>> _mutexes;
	std::mutex _mutex;
	std::unique_lock<std::shared_mutex> UniqueLock( const string& key )noexcept
	{
		unique_lock l{_mutex};

		auto p = _mutexes.find( key );
		auto pKeyMutex = p == _mutexes.end() ? _mutexes.emplace( key, make_shared<std::shared_mutex>() ).first->second : p->second;
		for( auto pExisting = _mutexes.begin(); pExisting != _mutexes.end();  )
			pExisting = pExisting->first!=key && pExisting->second.use_count()==1 && pExisting->second->try_lock() ? _mutexes.erase( pExisting ) : std::next( pExisting );
		l.unlock();
		return unique_lock{ *pKeyMutex };
	}
/*	std::shared_lock<std::shared_mutex> SharedLock( const string& key )noexcept
	{
		std::unique_ptr<std::unique_lock<std::shared_mutex>>
	}*/
}