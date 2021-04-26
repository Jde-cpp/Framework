#include "Cache.h"
#define var const auto
namespace Jde
{
	sp<Cache> _pInstance;
	sp<Cache> Cache::GetInstance()noexcept { return _pInstance; }
	void Cache::CreateInstance()noexcept
	{
		ASSERT( !_pInstance );
		_pInstance = sp<Cache>( new Cache{} );
		IApplication::AddShutdown( _pInstance );
	}
	void Cache::Shutdown()noexcept
	{
		DBG0( "Cache::Shutdown"sv );
		_pInstance = nullptr;
	}

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