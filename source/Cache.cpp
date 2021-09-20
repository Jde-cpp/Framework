#include "Cache.h"
#define var const auto
namespace Jde
{
	Cache _instance;
	Cache& Cache::Instance()noexcept { return _instance; }

	bool Cache::InstanceClear( sv name )noexcept
	{
		unique_lock l{_cacheLock};
		auto p = _cache.find( name );
		var erased = p!=_cache.end();
		if( erased )
		{
			_cache.erase( p );
			TRACE( "Cache::{} erased={}"sv, name, erased );
		}
		return erased;
	}
}