#pragma once
#include "application/Application.h"

namespace Jde
{
	struct Cache final : public Jde::IShutdown
	{
		~Cache(){ DBG0("~Cache"); }
		static void CreateInstance()noexcept;
		void Shutdown()noexcept;// override;

		template<class T>
		static sp<T> Get( const string& name )noexcept;
		template<class T>
		static sp<T> Set( const string& name, sp<T> pValue )noexcept;
	private:
		template<class T>
		sp<T> InstanceGet( const string& name )noexcept;

		template<class T>
		sp<T> InstanceSet( const string& name, sp<T> pValue )noexcept;

		Cache()noexcept{};
		JDE_NATIVE_VISIBILITY static sp<Cache> GetInstance()noexcept;
		map<string,sp<void>> _cache; shared_mutex _cacheLock;
	};

	template<class T>
	sp<T> Cache::Get( const string& name )noexcept
	{
		auto pInstance = GetInstance();
		return pInstance ? pInstance->InstanceGet<T>( name ) : sp<T>{};
	}
	
	template<class T>
	sp<T> Cache::InstanceGet( const string& name )noexcept
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
			unique_lock l{_cacheLock};
			pValue = std::static_pointer_cast<T>( _cache.try_emplace( name, make_shared<T>() ).first->second );
		}
		return pValue;
	}
	template<class T>
	sp<T> Cache::Set( const string& name, sp<T> pValue )noexcept
	{
		auto pInstance = GetInstance();
		return pInstance ? pInstance->InstanceSet<T>( name, pValue ) : pValue;
	}

	template<class T>
	sp<T> Cache::InstanceSet( const string& name, sp<T> pValue )noexcept
	{
		unique_lock l{_cacheLock};
		if( !pValue )
		{
			const bool erased = _cache.erase( name );
			TRACE( "Cache::{} erased={}", name, erased );
		}
		else
		{
			_cache[name] = pValue;
			TRACE( "Cache::{} set", name );
		}
		return pValue;
	}
}