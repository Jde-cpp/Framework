#pragma once
#include <map>
#include <shared_mutex>

#define SLOCK shared_lock<shared_mutex> l( Mutex );
#define LOCK unique_lock<shared_mutex> l( Mutex );
namespace Jde
{
	using std::map;
	using std::set;
	using std::shared_lock;
	using std::unique_lock;

#pragma region Map
	template<typename TKey, typename TValue>
	class Map : map<TKey,sp<TValue>>
	{
	public:
		typedef map<TKey,sp<TValue>> base;
		Map()=default;
		Map( const Map& copy )=delete;
		void erase( const TKey& item )noexcept{ LOCK base::erase( item ); }
		void clear()noexcept{ LOCK base::clear(); }
		//base Find( std::function<bool(const TValue&)> func )const;
		sp<TValue> Find( const TKey& key )const noexcept;
		sp<TValue> FindFirst( std::function<bool(const TValue&)> where );
		uint ForEach( std::function<void(const TKey&, const TValue&)>& )const;
		uint ForEach( std::function<void(const TKey&, TValue&)>& );
		template<class... Args >
		sp<TValue> emplace( Args&&... args )noexcept;

		mutable std::shared_mutex Mutex;
	private:
	};

	template<typename TKey, typename TValue>
	template<class... Args >
	sp<TValue> Map<TKey,TValue>::emplace( Args&&... args )noexcept
	{
		LOCK
		const auto result = base::emplace( args... );
		return result.first->second;
	}

	template<typename TKey, typename TValue>
	std::shared_ptr<TValue> Map<TKey,TValue>::Find( const TKey& key )const noexcept
	{
 		SLOCK
		const auto pItem = base::find( key );
		return pItem==base::end() ? nullptr : pItem->second;
	}

	template<typename TKey, typename TValue>
	uint Map<TKey,TValue>::ForEach( std::function<void(const TKey&, const TValue&)>& fncn )const
	{
 		SLOCK
		for( const auto& [id, pValue] : *this )
			fncn( id, *pValue );
		return base::size();
	}

	template<typename TKey, typename TValue>
	uint Map<TKey,TValue>::ForEach( std::function<void(const TKey&, TValue&)>& fncn )
	{
 		SLOCK
		for( const auto& [id, pValue] : *this )
			fncn( id, *pValue );
		return base::size();
	}

	template<typename TKey, typename TValue>
	shared_ptr<TValue> Map<TKey,TValue>::FindFirst( std::function<bool(const TValue&)> where )
	{
 		SLOCK
		shared_ptr<TValue> pFound;
		for( const auto& idValuePtr : *this )
		{
			if( where(*idValuePtr.second) )
			{
				pFound = idValuePtr.second;
				break;
			}
		}
		return pFound;
	}

#pragma endregion
namespace Collections//TODO refactor remove collections
{
	#pragma region MapValue
	template<typename TKey, typename TValue>
	class MapValue : private map<TKey,TValue>
	{
	public:
		typedef map<TKey,TValue> base;
		typedef unique_lock<shared_mutex> ULock;
		MapValue( )noexcept=default;
		MapValue( const MapValue& copy )=delete;
		bool erase( const TKey& item )noexcept{ LOCK return base::erase( item )>0; }
		typename base::iterator erase( const ULock&, typename base::iterator p )noexcept{ return base::erase( p ); }
		void clear()noexcept{ LOCK base::clear(); }
		base Find( std::function<bool(const TValue&)> func )const noexcept;
		TValue Find( TKey key, TValue dflt )const noexcept;
		bool contains( TKey key )const noexcept{SLOCK return base::find(key)!=base::end(); }

		typename base::iterator begin( const ULock& )noexcept{ return base::begin(); }
		typename base::iterator end( const ULock& )noexcept{ return base::end(); }

		template<class... Args> bool LockEmplace( const ULock&, Args&&... args )noexcept{ return base::emplace( args... ).second; }
		template<class... Args> bool emplace( Args&&... args )noexcept{ LOCK return LockEmplace( l, args... ); }
		template<class... Args> bool try_emplace( const TKey& k, Args&&... args )noexcept{ LOCK return base::try_emplace( k, args... ).second; }
		mutable std::shared_mutex Mutex;
	};

	template<typename TKey, typename TValue>
	map<TKey,TValue> MapValue<TKey,TValue>::Find( std::function<bool(const TValue&)> func )const noexcept
	{
		MapValue<TKey,TValue>::base results;
		SLOCK
		for( const auto& keyValue : *this )
		{
			if( func(keyValue.second) )
				results.emplace(keyValue.first, keyValue.second );
		}
		return results;
	}

	template<typename TKey, typename TValue>
	TValue MapValue<TKey,TValue>::Find( TKey key, TValue dflt )const noexcept
	{
		SLOCK
		const auto pKeyValue = base::find( key );
		return pKeyValue==base::end() ? dflt : pKeyValue->second;
	}
	#pragma endregion
	#undef LOCK
	#undef SLOCK
}}