﻿#pragma once
#ifndef COLLECTIONS_H
#define COLLECTIONS_H
#include <algorithm>
#include <forward_list>
#include <sstream>
#include <jde/framework/io/crc.h>

#define let const auto
namespace Jde{
	Ŧ FindDefault( const T& collection, typename T::key_type key )ι->typename T::mapped_type{
		auto pItem = collection.find( key );
		return pItem==collection.end() ? typename T::mapped_type{} : pItem->second;
	}
	Ŧ Find( const T& map, typename T::key_type key )ι->typename std::optional<typename T::mapped_type>{
		auto p = map.find( key );
		return p==map.end() ? std::optional<typename T::mapped_type>{} : p->second;
	}

	Ŧ FindKey( const T& collection, const typename T::mapped_type& value )ι->optional<typename T::key_type>{
		auto pEnd = collection.end();
		auto p = boost::container::find_if( collection.begin(), pEnd, [&value](let& x)ι->bool{ return x.second==value; } );
		return p==pEnd ? nullopt : optional<typename T::key_type>{ p->first };
	}

	Ŧ EmplaceShared( T& map, typename T::key_type key )ι->typename T::mapped_type&{
		auto p = map.find( key );
		if( p==map.end() )
			p = map.try_emplace( key, ms<typename T::mapped_type::element_type>() ).first;
		return p->second;
	}

	Ŧ EmplaceUnique( T& map, typename T::key_type key )ι->typename T::mapped_type&{
		auto p = map.find( key );
		if( p==map.end() )
			p = map.try_emplace( key, mu<typename T::mapped_type::element_type>() ).first;
		return p->second;
	}

	#define LAZY( x ) LazyWrap{ [&](){ return x;} }//doesn't work in ms
	template<class F> struct LazyWrap{ //https://stackoverflow.com/questions/62577415/lazy-argument-evaluation-for-try-emplace
		LazyWrap( F&& f )ι:_f(f) {}
		Τ operator T() { return _f(); }
		F _f;
	};

	Τ struct SPCompare{
		α operator()( const sp<T>& a, const sp<T>& b )Ι->bool{ return *a < *b; }
		α operator()(const sp<T>& a, const T& b)Ι->bool{ return *a < b; }
		α operator()( const T& a, const sp<T>& b)Ι->bool{ return a < *b; }
	};


	Ŧ Replace( T& map, const typename T::key_type& key, typename T::mapped_type&& value )ι->void{
		if( auto p = map.find(key); p!=map.end() )
			p->second = move( value );
		else
			map.emplace( key, move(value) );
	}
}
namespace Jde::Collections{
	uint32 Crc( const vector<string> values )ι;
	std::vector<uint> Indexes( const std::vector<string>& population, const std::vector<string>& subset )ι;

	template<typename Y, typename T>
	Y Map( const T& map, function<void( const typename T::key_type&, const typename T::mapped_type&, Y&)> f ){
		Y y;
		std::for_each( map.begin(), map.end(), [f,&y](let& kv){ f(kv.first,kv.second,y);} );
		return y;
	}

	ẗ Invert( const flat_map<K,V>& map )ι->flat_map<V,K>{
		flat_map<V,K> results;
		for_each( map.begin(), map.end(), [&results](let& x){results.emplace(x.second,x.first); } );
		return results;
	}

	ẗ Keys( const std::map<K,V>& map )ι->up<std::set<K>>{
		auto pResults = mu<std::set<K>>();
		for( const auto& keyValue : map )
			pResults->emplace( keyValue.first );

		return pResults;
	}

	ẗ Keys( const flat_map<K,V>& map, function<bool(const V&)> f )ι->flat_set<K>{
		flat_set<K> y;
		for( let& kv : map ){
			if( f(kv.second) )
				y.emplace( kv.first );
		}
		return y;
	}

	Ŧ Values( const T& map )ι->std::vector<typename T::mapped_type>{
		std::vector<typename T::mapped_type> y;
		for( const auto& keyValue : map )
			y.push_back( keyValue.second );

		return y;
	}

	template<template<class> class TCollection, class T>
	void ForEach( TCollection<T>& values, std::function<void(const T& t)> function ){
		std::for_each( values.begin(), values.end(), function );
	}

	ẗ ForEachMap( const std::map<K,V>& map, std::function<void(const K& key, const V& value)> func )ι->void{
		for( const auto& keyValue : map )
			func( keyValue.first, keyValue.second );
	}

	ẗ ForEachValue( const std::map<K,V>& map, std::function<void(const V& value)> func )ι->void{
		for( const auto& keyValue : map )
			func( keyValue.second );
	}

	template<typename T>
	std::size_t Find( std::vector<T> allValues, const T& find ){
		return Find<T,std::size_t>( allValues, std::vector<T>{find} )[0];
	}

	template<typename T,typename SizeType=std::size_t>
	std::vector<SizeType> Find( std::vector<T> allValues, std::vector<T> selectedValues ){
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
	std::vector<T> Combine( const std::vector<T>& initial, const std::initializer_list<const std::vector<T>*>& additions, std::size_t offset=0 ){
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

	ẗ FindFirst( const std::map<K,sp<V>>& collection, std::function<bool(const V&)> func )ι->const sp<V>{
		sp<V> pValue{nullptr};
		for( const auto& keyValuePtr : collection ){
			if( func(*keyValuePtr.second) ){
				pValue = keyValuePtr.second;
				break;
			}
		}
		return pValue;
	}

	ẗ FindFirst( std::map<K,sp<V>>& collection, std::function<bool(const V&)> func )ι->sp<V>{
		sp<V> pValue{nullptr};
		for( const auto& keyValuePtr : collection ){
			if( func(*keyValuePtr.second) ){
				pValue = keyValuePtr.second;
				break;
			}
		}
		return pValue;
	}

	ẗ Erase( std::map<K,sp<V>>& collection, std::function<bool(const V&)> func )ι->void{
		std::forward_list<K> doomed;
		for( const auto& keyValuePtr : collection ){
			if( func(*keyValuePtr.second) )
				doomed.push_front( keyValuePtr.first );
		}
		for( const auto& key : doomed )
			collection.erase( key );
	}


	template<typename T>
	α ContainsAll( const std::set<T>& universe, const std::set<T>& set )ι->bool{
		bool in = set.size() <= universe.size();
		if( in ){
			for( const auto& item : set ){
				in = universe.find(item)!=universe.end();
				if( !in )
					break;
			}
		}
		return in;
	}


	inline std::vector<uint> Indexes( const std::vector<string>& population, const std::vector<string>& subset )ι{
		std::vector<uint> results; results.reserve( subset.size() );
		for( const auto& subsetItem : subset ){
			const uint index = find( population.begin(), population.end(), subsetItem ) - population.begin();
			if( index==population.size() )
				Error( ELogTags::App, "Could not index {}", subsetItem );
			results.push_back( index );
		}
		return results;
	}

	template<template<class, class,class...> class M, class K, class V, class... Ts>
	up<V>& InsertUnique( M<K,up<V>,Ts...>& map, const K& key )ι{
		auto p = map.find( key );//M<K,std::up<V>>::iterator
		if( p == map.end() )
			p = map.emplace( key, mu<V>() ).first;
		return p->second;
	}
}
#undef let
#endif