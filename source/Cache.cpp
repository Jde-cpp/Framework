#include "Cache.h"
#define var const auto
namespace Jde
{
	Cache _instance;
	α Cache::Instance()noexcept->Cache& { return _instance; }

	α Cache::InstanceClear( sv name )noexcept->bool
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

	std::map<string,tuple<double,TP>> _cacheDouble; shared_mutex _cacheDoubleLock;
	α Cache::Double( string id )noexcept->double
	{
		sl l{ _cacheDoubleLock };
		auto p = _cacheDouble.find( id ); 
		return p==_cacheDouble.end() ? std::nan( "" ) : get<0>( p->second );
	}
	α Cache::SetDouble( string id, double v, TP expiration )noexcept->bool
	{
		if( expiration==TP::max() )
		{
			if( var d{ Duration(id) }; d!=Duration::max() )
				expiration =  Clock::now()+d;
		}
		auto r = _cacheDouble.emplace( move(id), make_tuple(v,expiration) ); 
		if( !r.second )
			r.first->second = make_tuple( v, expiration );
		return r.second;
	}
}