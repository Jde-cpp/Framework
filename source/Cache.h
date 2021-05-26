#pragma once
#include <jde/App.h>
#include <jde/Log.h>
#include <jde/Str.h>
#include <jde/Exports.h>

namespace Jde
{
#define var const auto
#define CALL(x,y) auto p = GetInstance(); return p ? p->x : y
	struct Cache final : public Jde::IShutdown
	{
		~Cache(){ if( HaveLogger() ) DBG0("~Cache"sv); }
		JDE_NATIVE_VISIBILITY static void CreateInstance()noexcept;
		void Shutdown()noexcept;// override;
		static bool Has( str name )noexcept;
		template<class T>
		static sp<T> TryGet( str name )noexcept;//returns non-null value.
		template<class T>
		static sp<T> Get( str name )noexcept(false);
		template<class T>
		static sp<T> Set( str name, sp<T> pValue )noexcept;
		JDE_NATIVE_VISIBILITY static auto Clear( sv name )noexcept->bool{ CALL(InstanceClear(name),false); }

		template<class K,class V> static optional<V> GetValue( sv cacheName, K id )noexcept;

	private:
		bool InstanceClear( sv name )noexcept;
		bool InstanceHas( str name )const noexcept{ shared_lock l{_cacheLock}; return _cache.find( name )!=_cache.end(); }
		template<class T> sp<T> InstanceGet( str name )noexcept(false);
		template<class K,class V>  optional<V> InstanceGetValue( sv cacheName, K id )noexcept;

		template<class T> sp<T> InstanceTryGet( str name )noexcept;

		template<class T>
		sp<T> InstanceSet( str name, sp<T> pValue )noexcept;

		Cache()noexcept{};
		JDE_NATIVE_VISIBILITY static sp<Cache> GetInstance()noexcept;
		map<string,sp<void>,std::less<>> _cache; mutable shared_mutex _cacheLock;
	};

	inline bool Cache::Has( str name )noexcept
	{
		auto pInstance = GetInstance();
		return pInstance && pInstance->InstanceHas( name );
	}
	//possible null return
	template<class T>
	sp<T> Cache::Get( str name )noexcept(false)
	{
		auto pInstance = GetInstance();
		if( !pInstance )
			THROW2( Exception("no cache instance.") );
		return pInstance->InstanceGet<T>( name );
	}
	//non-null return TODO change name/pass in default args.
	template<class T>
	sp<T> Cache::TryGet( str name )noexcept
	{
		auto pInstance = GetInstance();
		return pInstance ? pInstance->InstanceTryGet<T>( name ) : make_shared<T>();
	}
	template<class T>
	sp<T> Cache::InstanceGet( str name )noexcept(false)
	{
		sp<T> pValue;
		shared_lock l{_cacheLock};
		auto p = _cache.find( name );
		//if( p==_cache.end() )
		//	THROW( Exception("'{}' not found in cache.", name) );
		return p==_cache.end() ? sp<T>{} : std::static_pointer_cast<T>( p->second );
	}

	template<class T>
	sp<T> Cache::InstanceTryGet( str name )noexcept
	{
		sp<T> pValue;
		shared_lock l{_cacheLock};
		auto p = _cache.find( name );
		if( p!=_cache.end() )
		{
			sp<void> pVoid = p->second;
			//auto pTemp = static_cast<T*>( pVoid.get() );
			pValue = std::static_pointer_cast<T>( pVoid );
		}
		else
		{
			l.unlock();
			unique_lock l2{_cacheLock};
			pValue = std::static_pointer_cast<T>( _cache.try_emplace( name, make_shared<T>() ).first->second );
		}
		return pValue;
	}
	template<class K,class V>  optional<V> Cache::GetValue( sv cacheName, K id )noexcept{ auto p = GetInstance(); return p ? p->GetValue<K,V>( cacheName, id ) : optional<V>{}; }
	template<class K,class V>  optional<V> Cache::InstanceGetValue( sv cacheName, K id )noexcept
	{
		sp<V> pValue;
		shared_lock l{_cacheLock};
		if( auto p = _cache.find( cacheName ); p!=_cache.end() )
		{
			var pMap = std::dynamic_pointer_cast<flat_map<K,V>>( p->second );
			if( pMap )
			{
				if( auto pItem = pMap->find( id ); pItem != pMap->end() )
					pValue = pItem->second;
			}
		}
		return pValue;
	}


	template<class T> sp<T> Cache::Set( str name, sp<T> pValue )noexcept
	{
		auto pInstance = GetInstance();
		return pInstance ? pInstance->InstanceSet<T>( name, pValue ) : pValue;
	}

	template<class T>
	sp<T> Cache::InstanceSet( str name, sp<T> pValue )noexcept
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
#undef CALL
}