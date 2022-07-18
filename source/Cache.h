#pragma once
#include <jde/App.h>
#include <jde/Log.h>
#include <jde/Str.h>
#include <jde/Exports.h>
#include "Settings.h"
#include "collections/Collections.h"

namespace Jde
{
#define var const auto
#define Φ Γ Ω
	struct Cache final //: public Jde::IShutdown
	{
		~Cache(){ if( HaveLogger() ) DBG("~Cache"sv); }
		Ω Has( str name )noexcept{ return Instance().InstanceHas( name ); }
		Ω Duration( str /*name*/ )noexcept{ return Settings::Get<Jde::Duration>( "cache/default/duration" ).value_or( Duration::max() ); }
		Ṫ Emplace( str name )noexcept->sp<T>{ return Instance().InstanceEmplace<T>( name ); }
		Ṫ Get( str name )noexcept{ return Instance().InstanceGet<T>(name); }
		Φ Double( string name )noexcept->double;
		Φ SetDouble( string name, double v, TP t=TP::max() )noexcept->bool;
		Ṫ Set( str name, sp<T> p )noexcept->sp<T>{ return Instance().InstanceSet<T>(name, p); }
		Φ Clear( sv name )noexcept->bool{ return Instance().InstanceClear( name ); }
		template<class K,class V> static α GetValue( str n, K id )noexcept->sp<V>{ return Instance().InstanceGetValue<K,V>( n, id ); }

	private:
		α InstanceClear( sv name )noexcept->bool;
		α InstanceHas( str name )const noexcept->bool{ shared_lock l{_cacheLock}; return _cache.find( name )!=_cache.end(); }
		Ŧ InstanceGet( str name )noexcept->sp<T>;
		ẗ InstanceGetValue( str n, K id )noexcept->sp<V>;
		Ŧ InstanceEmplace( str name )noexcept->sp<T>;
		Ŧ InstanceSet( str name, sp<T> pValue )noexcept->sp<T>;
		Φ Instance()noexcept->Cache&;
		std::map<string,sp<void>,std::less<>> _cache; mutable shared_mutex _cacheLock;
		Φ LogLevel()->const LogTag&;
	};

	Ŧ Cache::InstanceGet( str name )noexcept->sp<T>
	{
		shared_lock l{_cacheLock};
		auto p = _cache.find( name );
		return p==_cache.end() ? sp<T>{} : std::static_pointer_cast<T>( p->second );
	}

	Ŧ Cache::InstanceEmplace( str name )noexcept->sp<T>
	{
		shared_lock l{_cacheLock};
		auto p = _cache.find( name );
		if( p==_cache.end() )
			p = _cache.try_emplace( name, make_shared<T>() ).first;
		return std::static_pointer_cast<T>( p->second );
	}

	ẗ Cache::InstanceGetValue( str cacheName, K id )noexcept->sp<V>
	{
		sp<V> pValue;
		shared_lock l{_cacheLock};
		if( auto p = _cache.find( cacheName ); p!=_cache.end() )
		{
			if( var pMap = std::static_pointer_cast<flat_map<K,sp<V>>>(p->second); pMap )
			{
				if( auto pItem = pMap->find( id ); pItem != pMap->end() )
					pValue = pItem->second;
			}
		}
		return pValue;
	}
#define _logLevel LogLevel()
	Ŧ Cache::InstanceSet( str name, sp<T> pValue )noexcept->	sp<T>
	{
		unique_lock l{_cacheLock};
		if( !pValue )
		{
			const bool erased = _cache.erase( name );
			TRACE( "Cache::{} erased={}"sv, name, erased );
		}
		else
		{
			_cache[name] = pValue;
			TRACE( "Cache::{} set"sv, name );
		}
		return pValue;
	}
#undef var
#undef Φ
#undef _logLevel
}