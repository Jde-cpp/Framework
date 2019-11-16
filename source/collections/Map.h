#pragma once
#include <map>
#include <shared_mutex>

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
		typedef map<TKey,sp<TValue>> Base;
		Map()=default;
		Map( const Map& copy )=delete;
		void erase( const TKey& item )noexcept{ unique_lock<std::shared_mutex> l( _mutex ); Base::erase( item ); }
		void clear()noexcept{ unique_lock<std::shared_mutex> l( _mutex ); Base::clear(); }
		//Base Find( std::function<bool(const TValue&)> func )const;
		sp<TValue> Find( const TKey& key )const noexcept;
		sp<TValue> FindFirst( std::function<bool(const TValue&)> where );
		uint ForEach( std::function<void(const TKey&, const TValue&)>& )const;
		uint ForEach( std::function<void(const TKey&, TValue&)>& );
		template<class... Args >
		sp<TValue> emplace( Args&&... args )noexcept;
	private:
		mutable std::shared_mutex _mutex;
	};
	
	template<typename TKey, typename TValue>
	template<class... Args >
	sp<TValue> Map<TKey,TValue>::emplace( Args&&... args )noexcept
	{
		std::unique_lock<std::shared_mutex> l;
		const auto result = Base::emplace( args... );
		return result.first->second;
	}

	template<typename TKey, typename TValue>
	std::shared_ptr<TValue> Map<TKey,TValue>::Find( const TKey& key )const noexcept 
	{
 		shared_lock<std::shared_mutex> l(_mutex);
		const auto pItem = Base::find( key );
		return pItem==Base::end() ? nullptr : pItem->second;
	}

	template<typename TKey, typename TValue>
	uint Map<TKey,TValue>::ForEach( std::function<void(const TKey&, const TValue&)>& fncn )const
	{
 		shared_lock<std::shared_mutex> l(_mutex);
		for( const auto& [id, pValue] : *this )
			fncn( id, *pValue );
		return Base::size();
	}

	template<typename TKey, typename TValue>
	uint Map<TKey,TValue>::ForEach( std::function<void(const TKey&, TValue&)>& fncn )
	{
 		shared_lock<std::shared_mutex> l(_mutex);
		for( const auto& [id, pValue] : *this )
			fncn( id, *pValue );
		return Base::size();
	}

	template<typename TKey, typename TValue>
	shared_ptr<TValue> Map<TKey,TValue>::FindFirst( std::function<bool(const TValue&)> where )
	{
 		shared_lock<std::shared_mutex> l(_mutex);
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
namespace Collections
{
	#pragma region MapValue
	template<typename TKey, typename TValue>
	class MapValue : map<TKey,TValue>
	{
	public:
		typedef map<TKey,TValue> Base;
		MapValue( )=default;
		MapValue( const MapValue& copy )=delete;
		bool erase( const TKey& item )noexcept{ unique_lock<std::shared_mutex> l( _mutex ); return Base::erase( item )>0; }
		void clear()noexcept{ unique_lock<std::shared_mutex> l( _mutex ); Base::clear(); }
		Base Find( std::function<bool(const TValue&)> func )const;
		TValue Find( TKey key, TValue dflt )const noexcept;

		template<class... Args >
		bool emplace( Args&&... args )noexcept;
	private:
		mutable std::shared_mutex _mutex;	
	};
	template<typename TKey, typename TValue>
	template<class... Args >
	bool MapValue<TKey,TValue>::emplace( Args&&... args )noexcept
	{
		std::unique_lock<std::shared_mutex> l;
		const auto result = Base::emplace( args... );
		return result.second;
	}
	template<typename TKey, typename TValue>
	map<TKey,TValue> MapValue<TKey,TValue>::Find( std::function<bool(const TValue&)> func )const
	{
		MapValue<TKey,TValue>::Base results;
		shared_lock<std::shared_mutex> l( _mutex ); 
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
		shared_lock<std::shared_mutex> l( _mutex ); 
		const auto pKeyValue = Base::find( key );
		return pKeyValue==Base::end() ? dflt : pKeyValue->second;
	}
	#pragma endregion
}}