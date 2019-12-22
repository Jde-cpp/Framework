#pragma once
#include <algorithm>
#include <forward_list>
#include <sstream>
#include "../io/Crc.h"

namespace Jde
{
	using std::map;
namespace Collections
{
	uint32 Crc( const vector<string> values )noexcept;
	std::vector<uint> Indexes( const std::vector<string>& population, const std::vector<string>& subset )noexcept;

	template<typename TKey, typename TValue>
	unique_ptr<std::set<TKey>> Keys( const map<TKey,TValue>& map )noexcept
	{
		auto pResults = make_unique<std::set<TKey>>();
		for( const auto& keyValue : map )
			pResults->emplace( keyValue.first );

		return pResults;
	}

	template<typename TKey, typename TValue>
	unique_ptr<std::vector<TValue>> Values( const map<TKey,TValue>& map )
	{
		auto pResults = make_unique<std::vector<TValue>>(); pResults->reserve( map.size() );
		for( const auto& keyValue : map )
			pResults->push_back( keyValue.second );

		return pResults;
	}

	template<template<class> class TCollection, class T>
	void ForEach( TCollection<T>& values, std::function<void(const T& t)> function )
	{
		std::for_each( values.begin(), values.end(), function );
	}

	template<typename TKey, typename TValue>
	void ForEachMap( const map<TKey,TValue>& map, std::function<void(const TKey& key, const TValue& value)> func )
	{
		for( const auto& keyValue : map )
			func( keyValue.first, keyValue.second );
	}

	template<typename TKey, typename TValue>
	void ForEachValue( const map<TKey,TValue>& map, std::function<void(const TValue& value)> func )
	{
		for( const auto& keyValue : map )
			func( keyValue.second );
	}

	template<typename TKey, typename TValue>
	const TValue& TryGetValue( const map<TKey,TValue>& map, const TKey& key, const TValue& deflt )
	{
		const auto& pItem = map.find(key);
		return pItem==map.end() ? deflt : *pItem;
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

	template<typename TKey,typename TValue>
	const shared_ptr<TValue> FindFirst( const std::map<TKey,shared_ptr<TValue>>& collection, std::function<bool(const TValue&)> func )
	{
		shared_ptr<TValue> pValue{nullptr};
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

	template<typename TKey,typename TValue>
	shared_ptr<TValue> FindFirst( std::map<TKey,shared_ptr<TValue>>& collection, std::function<bool(const TValue&)> func )
	{
		shared_ptr<TValue> pValue{nullptr};
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

	template<typename TKey,typename TValue>
	void Erase( std::map<TKey,shared_ptr<TValue>>& collection, std::function<bool(const TValue&)> func )
	{
		std::forward_list<TKey> doomed;
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
				{
					//GetDefaultLogger()->debug( "{}", item.DateDisplay() );
					//for( const auto& item2 : universe )
						//GetDefaultLogger()->debug( "{}", item2.DateDisplay() );
					break;
				}
			}
		}
		return in;
	}

	inline uint32 Crc( const vector<string> values )noexcept
	{
		boost::crc_32_type result;
		std::ostringstream os;
		for( const auto& value : values )
		{
			result.process_bytes( value.c_str(), value.size() );
			os << value;
		}	
		uint32 checksum = result.checksum();

		assert( checksum==IO::Crc::Calc32(os.str()) );
		return checksum;
	}
	inline std::vector<uint> Indexes( const std::vector<string>& population, const std::vector<string>& subset )noexcept
	{
		std::vector<uint> results; results.reserve( subset.size() );
		for( const auto& subsetItem : subset )
		{
			const uint index = find( population.begin(), population.end(), subsetItem ) - population.begin();
			if( index==population.size() )
				ERRN( "Could not index {}", subsetItem );
			results.push_back( index );
		}
		return results;
	}
}}
