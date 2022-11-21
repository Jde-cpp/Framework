#include "Cache.h"
#define var const auto
namespace Jde
{
	std::map<string,tuple<double,TP>> _cacheDouble; shared_mutex _cacheDoubleLock;

	Φ Cache2::Has( str id )ι{ return sl l{_cacheLock}; return _cache.find( id )!=_cache.end(); }
	Φ Cache2::Duration( str id )ι->Duration
	{
		return Settings::Get<Duration>( format("cache/{}/duration",id) ).value_or(
			Settings::Get<Duration>( "cache/default/duration" ).value_or( Duration::max() );
	}

	α Cache2::Double( str id )ι->double
	{
		sl l{ _cacheDoubleLock };
		auto p = _cacheDouble.find( id );
		return p==_cacheDouble.end() ? std::nan( "" ) : get<0>( p->second );
	}

	//Find out about this...
	Φ Cache2::SetDouble( str id, double v, TP t=TP::max() )ι->bool
	{
		if( t==TP::max() )
		{
			if( var d{ Duration(id) }; d!=Duration::max() )
				t =  Clock::now()+d;
		}
		auto r = _cacheDouble.emplace( move(id), make_tuple(v,t) );
		if( !r.second )
			r.first->second = make_tuple( v, t );
		return r.second;
	}

	static const LogTag& _logLevel = Logging::TagLevel( "cache" );
	α Cache::LogLevel()->const LogTag&{ return _logLevel; }
	Cache _instance;
	α Cache::Instance()ι->Cache& { return _instance; }

	α Cache::InstanceClear( sv name )ι->bool
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

	α Cache::Double( string id )ι->double
	{
		sl l{ _cacheDoubleLock };
		auto p = _cacheDouble.find( id );
		return p==_cacheDouble.end() ? std::nan( "" ) : get<0>( p->second );
	}
	α Cache::SetDouble( string id, double v, TP expiration )ι->bool
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