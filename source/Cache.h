#pragma once
#include "application/Application.h"

namespace Jde
{
	struct Cache final : public Jde::IShutdown
	{
		~Cache(){ if( HaveLogger() ) DBG0("~Cache"sv); }
		static void CreateInstance()noexcept;
		void Shutdown()noexcept;// override;
		static bool Has( const string& name )noexcept;
		template<class T>
		static sp<T> TryGet( const string& name )noexcept;//returns non-null value.
		template<class T>
		static sp<T> Get( const string& name )noexcept(false);
		template<class T>
		static sp<T> Set( const string& name, sp<T> pValue )noexcept;
	private:
		bool InstanceHas( const string& name )const noexcept{ shared_lock l{_cacheLock}; return _cache.find( name )!=_cache.end(); }
		template<class T>
		sp<T> InstanceGet( const string& name )noexcept(false);
		template<class T>
		sp<T> InstanceTryGet( const string& name )noexcept;

		template<class T>
		sp<T> InstanceSet( const string& name, sp<T> pValue )noexcept;

		Cache()noexcept{};
		JDE_NATIVE_VISIBILITY static sp<Cache> GetInstance()noexcept;
		map<string,sp<void>> _cache; mutable shared_mutex _cacheLock;
	};

	inline bool Cache::Has( const string& name )noexcept
	{
		auto pInstance = GetInstance();
		return pInstance && pInstance->InstanceHas( name );
	}
	//possible null return
	template<class T>
	sp<T> Cache::Get( const string& name )noexcept(false)
	{
		auto pInstance = GetInstance();
		if( !pInstance )
			THROW( Exception("no cache instance.") );
		return pInstance->InstanceGet<T>( name );
	}
	//non-null return TODO change name/pass in default args.
	template<class T>
	sp<T> Cache::TryGet( const string& name )noexcept
	{
		auto pInstance = GetInstance();
		return pInstance ? pInstance->InstanceTryGet<T>( name ) : make_shared<T>();
	}
	template<class T>
	sp<T> Cache::InstanceGet( const string& name )noexcept(false)
	{
		sp<T> pValue;
		shared_lock l{_cacheLock};
		auto p = _cache.find( name );
		//if( p==_cache.end() )
		//	THROW( Exception("'{}' not found in cache.", name) );
		return p==_cache.end() ? sp<T>{} : std::static_pointer_cast<T>( p->second );
	}

	template<class T>
	sp<T> Cache::InstanceTryGet( const string& name )noexcept
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
			TRACE( "Cache::{} erased={}"sv, name, erased );
		}
		else
		{
			_cache[name] = pValue;
			TRACE( "Cache::{} set"sv, name );
		}
		return pValue;
	}
}