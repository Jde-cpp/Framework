#pragma once
#include <algorithm>
#include <forward_list>
#include <sstream>
#include <jde/io/Crc.h>
#include <jde/Log.h>

#define var const auto
namespace Jde
{
	template<typename T>
	typename T::mapped_type Find( const T& collection, typename T::key_type key )
	{
		auto pItem = collection.find( key );
		return pItem==collection.end() ? typename T::mapped_type{} : pItem->second;
	}

	template<typename T>
	typename T::mapped_type Find( const sp<T>& p, typename T::key_type key )
	{
		auto pCopy = p;
		return pCopy ? Find( *pCopy, key ) : typename T::mapped_type{};
	}

	ⓣ EmplaceShared( T& map, typename T::key_type key )->typename T::mapped_type&
	{
		auto p = map.find( key );
		if( p==map.end() )
			p = map.try_emplace( key, make_shared<typename T::mapped_type::element_type>() ).first;
		return p->second;
	}


	#define LAZY( x ) LazyWrap{ [&](){ return x;} }//doesn't work in ms
	template<class F> struct LazyWrap //https://stackoverflow.com/questions/62577415/lazy-argument-evaluation-for-try-emplace
	{
		LazyWrap( F&& f ):_f(f) {}
		template<class T> operator T() { return _f(); }
		F _f;
	};

	template <typename T>
	struct SPCompare
	{
		//using is_transparent = void;
		bool operator()( const sp<T>& a, const sp<T>& b )const{
			return *a < *b;
		}

		bool operator() (const sp<T>& a, const T& b) const
		{
			return *a < b;
		}

		bool operator()( const T& a, const sp<T>& b) const
		{
			return a < *b;
		}
	};

	ẗ TryGetValue( const flat_map<K,V>& map, const K& key, V deflt )->V
	{
		const auto& pItem = map.find(key);
		return pItem==map.end() ? deflt : *pItem;
	}

namespace Collections
{
	uint32 Crc( const vector<string> values )noexcept;
	std::vector<uint> Indexes( const std::vector<string>& population, const std::vector<string>& subset )noexcept;

	template<typename Y, typename T>
	Y Map( const T& map, function<void( const typename T::key_type&, const typename T::mapped_type&, Y&)> f )
	{
		Y y;
		std::for_each( map.begin(), map.end(), [f,&y](var& kv){ f(kv.first,kv.second,y);} );
		return y;
	}

	ẗ Invert( const flat_map<K,V>& map )noexcept->flat_map<V,K>
	{
		flat_map<V,K> results;
		for_each( map.begin(), map.end(), [&results](var& x){results.emplace(x.second,x.first); } );
		return results;
	}

	ẗ Keys( const map<K,V>& map )noexcept->up<std::set<K>>
	{
		auto pResults = make_unique<std::set<K>>();
		for( const auto& keyValue : map )
			pResults->emplace( keyValue.first );

		return pResults;
	}

	ẗ Values( const map<K,V>& map )->up<std::vector<V>>
	{
		auto pResults = make_unique<std::vector<V>>(); pResults->reserve( map.size() );
		for( const auto& keyValue : map )
			pResults->push_back( keyValue.second );

		return pResults;
	}

	template<template<class> class TCollection, class T>
	void ForEach( TCollection<T>& values, std::function<void(const T& t)> function )
	{
		std::for_each( values.begin(), values.end(), function );
	}

	ẗ ForEachMap( const map<K,V>& map, std::function<void(const K& key, const V& value)> func )->void
	{
		for( const auto& keyValue : map )
			func( keyValue.first, keyValue.second );
	}

	ẗ ForEachValue( const map<K,V>& map, std::function<void(const V& value)> func )->void
	{
		for( const auto& keyValue : map )
			func( keyValue.second );
	}

	template<typename T>
	std::size_t Find( std::vector<T> allValues, const T& find )
	{
		return Find<T,std::size_t>( allValues, std::vector<T>{find} )[0];
	}

	template<typename T,typename SizeType=std::size_t>
	std::vector<SizeType> Find( std::vector<T> allValues, std::vector<T> selectedValues )
	{
		std::vector<SizeType> indexes;
		for( const T& selection : selectedValues )
		{
			const auto iterator = std::find_if( allValues.begin(), allValues.end(), [&selection](const T& item){ return item==selection;} );
			if( iterator==allValues.end() )
				throw "wtf";
			indexes.push_back( SizeType(std::distance(allValues.begin(), iterator)) );
		}
		return indexes;
	}

	template<typename T>
	std::vector<T> Combine( const std::vector<T>& initial, const std::initializer_list<const std::vector<T>*>& additions, std::size_t offset=0 )
	{
		std::vector<T> result;
		std::size_t size=initial.size();
		for( const auto& pAppend : additions )
			size+=pAppend->size()-offset;

		result.reserve(size);
		result.insert( result.end(), initial.begin(), initial.end() );
		for( const auto& pAppend : additions )
		{
			auto pItem = pAppend->begin();
			for(std::size_t i=0; i<offset; ++i )
				++pItem;
			result.insert( result.end(), pItem, pAppend->end() );
		}
		return result;
	}

	ẗ FindFirst( const std::map<K,sp<V>>& collection, std::function<bool(const V&)> func )->const sp<V>
	{
		sp<V> pValue{nullptr};
		for( const auto& keyValuePtr : collection )
		{
			if( func(*keyValuePtr.second) )
			{
				pValue = keyValuePtr.second;
				break;
			}
		}
		return pValue;
	}

	ẗ FindFirst( std::map<K,sp<V>>& collection, std::function<bool(const V&)> func )->sp<V>
	{
		sp<V> pValue{nullptr};
		for( const auto& keyValuePtr : collection )
		{
			if( func(*keyValuePtr.second) )
			{
				pValue = keyValuePtr.second;
				break;
			}
		}
		return pValue;
	}

	ẗ Erase( std::map<K,sp<V>>& collection, std::function<bool(const V&)> func )->void
	{
		std::forward_list<K> doomed;
		for( const auto& keyValuePtr : collection )
		{
			if( func(*keyValuePtr.second) )
				doomed.push_front( keyValuePtr.first );
		}
		for( const auto& key : doomed )
			collection.erase( key );
	}


	template<typename T>
	bool ContainsAll( const std::set<T>& universe, const std::set<T>& set )
	{
		bool in = set.size() <= universe.size();
		if( in )
		{
			for( const auto& item : set )
			{
				in = universe.find(item)!=universe.end();
				if( !in )
					break;
			}
		}
		return in;
	}


	inline std::vector<uint> Indexes( const std::vector<string>& population, const std::vector<string>& subset )noexcept
	{
		std::vector<uint> results; results.reserve( subset.size() );
		for( const auto& subsetItem : subset )
		{
			const uint index = find( population.begin(), population.end(), subsetItem ) - population.begin();
			if( index==population.size() )
				ERR( "Could not index {}"sv, subsetItem );
			results.push_back( index );
		}
		return results;
	}

	template<template<class, class,class...> class M, class K, class V, class... Ts>
	up<V>& InsertUnique( M<K,up<V>,Ts...>& map, const K& key )noexcept
	{
		auto p = map.find( key );//M<K,std::up<V>>::iterator
		if( p == map.end() )
			p = map.emplace( key, make_unique<V>() ).first;
		return p->second;
	}

	template<template<class, class,class...> class M, class K, class V, class... Ts>
	V& InsertShared( M<K,sp<V>,Ts...>& map, const K& key )noexcept
	{
		auto p = map.find( key );//M<K,sp<V>>::iterator
		if( p == map.end() )
			p = map.emplace( key, make_shared<V>() ).first;
		return *p->second;
	}
}}
#undef var
