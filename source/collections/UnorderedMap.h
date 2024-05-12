#pragma once
#ifndef UnorderedMap_H
#define UnorderedMap_H
#include <set>
#include <unordered_map>
#include <forward_list>
#include <shared_mutex>
#include <functional>
#include <jde/TypeDefs.h>

#define LOCK unique_lock<shared_mutex> l(_mutex)
#define var const auto
namespace Jde
{
	using std::unordered_map;
	using std::set;
	using std::shared_lock;
	using std::unique_lock;
	using std::function;
}
namespace Jde::Collections
{
	template<typename TKey, typename TValue>
	struct UnorderedMap : private std::unordered_map<TKey,sp<TValue>>
	{
		typedef std::unordered_map<TKey,sp<TValue>> base;
		UnorderedMap()=default;
		UnorderedMap( const UnorderedMap& copy );
		α operator=( UnorderedMap&& x )->UnorderedMap&;
		bool erase( const TKey& item )ι;
		bool eraseIf( const TKey& item, function<bool(const TValue&)> func )ι;
		uint eraseIf( function<bool(const TValue&)> func )ι;
		α clear()ι{ unique_lock<shared_mutex> l(_mutex); base::clear(); }
		uint size()Ι;
		bool IfNone( function<bool(const TKey&,const TValue&)> ifFunction, function<void()> function );
		sp<TValue> Find( const TKey& key )Ι;
		sp<TValue> FindFirst( function<bool(const TValue&)> where );
		bool ForFirst( function<bool(const TKey&, TValue&)> func );
		uint ForEach( function<void(const TKey&, const TValue&)> )const;
		uint ForEach( function<void(const TKey&, TValue&)> );
		template<class... Args>
		std::pair<std::pair<TKey,sp<TValue>>,bool> emplace( Args&&... args );
		template<class... Args>
		bool Insert( function<void(TValue&)> afterInsert, Args&&... args );
		bool TryEmplace( function<void(TValue&)> afterInsert, const TKey& key );
		α Update( const TKey& key, function<void(TValue&)>& func )->void;
		α Where( const TKey& key, function<void(const TValue&)> func )->void;
		template<class... Args >
		α AddOrUpdate( TKey key, function<sp<TValue>()> add, function<void(TValue&)> update )->void;
		sp<std::forward_list<sp<TValue>>> Values()const;
		bool Set( const TKey& key, sp<TValue> pValue )ι;
		std::unique_lock<shared_mutex> Lock()ι{ return std::unique_lock<shared_mutex>{_mutex}; }
	private:
		mutable shared_mutex _mutex;
	};

	template<class TKey, class TValue> using UnorderedMapPtr = sp<UnorderedMap<TKey,TValue>>;

	template<typename TKey, typename TValue>
	UnorderedMap<TKey,TValue>::UnorderedMap( const UnorderedMap& copy )
	{
		std::unique_lock<shared_mutex> l( copy._mutex );
		for( const auto& [key,value] : copy )
			base::emplace( key, value );
	}

	ẗ UnorderedMap<K,V>::operator=( UnorderedMap&& x )->UnorderedMap&
	{
		LOCK;
		base::clear();
		unique_lock<shared_mutex> l2{ x._mutex };
		for( var& i : x )
			base::emplace( i.first, i.second );
		((base&)x).clear();
		return *this;
	}

	template<typename TKey, typename TValue>
	α UnorderedMap<TKey,TValue>::Update( const TKey& key, function<void(TValue& value)>& func )->void
	{
 		shared_lock<shared_mutex> l(_mutex);
		auto pItem = base::find( key );
		if( pItem!=base::end() )
			func( *pItem->second );
	}

	template<typename TKey, typename TValue>
	α UnorderedMap<TKey,TValue>::Where( const TKey& key, function<void(const TValue&)> func )->void
	{
		shared_lock<shared_mutex> l(_mutex);
		auto pItem = base::find( key );
		if( pItem!=base::end() )
			func( *pItem->second );
	}

	template<typename TKey, typename TValue>
	template<class... Args >
	α UnorderedMap<TKey,TValue>::AddOrUpdate( TKey key, function<sp<TValue>()> add, function<void(TValue&)> update )->void
	{
 		unique_lock<shared_mutex> l(_mutex);
		auto pItem = base::find( key );
		if( pItem==base::end() )
			base::emplace( key, add() );
		else
			update( *pItem->second );
	}


	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::erase( const TKey& item )ι
	{
		unique_lock<shared_mutex> l( _mutex );
		return base::erase( item )>0;
	}
	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::eraseIf( const TKey& key, function<bool(const TValue&)> func )ι
	{
		unique_lock<shared_mutex> l( _mutex );
		auto pItem = base::find( key );
		bool erase = pItem!=base::end() && func( *pItem->second );
		if( erase )
			base::erase( pItem );
		return erase;
	}
	template<typename TKey, typename TValue>
	uint UnorderedMap<TKey,TValue>::eraseIf( function<bool(const TValue&)> func )ι
	{
		unique_lock<shared_mutex> l( _mutex );
		uint count = 0;
		for( auto pItem = base::begin(); pItem!=base::end(); )
		{
			if( func(*pItem->second) )
			{
				pItem = base::erase( pItem );
				++count;
			}
			else
				++pItem;
		}
		return count;
	}
	template<typename TKey, typename TValue>
	uint UnorderedMap<TKey,TValue>::size()Ι
	{
		shared_lock<shared_mutex> l(_mutex);
		return base::size();
	}

	template<typename TKey, typename TValue>
	template<class... Args >
	std::pair<std::pair<TKey,sp<TValue>>,bool> UnorderedMap<TKey,TValue>::emplace( Args&&... args )
	{
		std::unique_lock<shared_mutex> l{_mutex};
		auto result = base::emplace( args... );
		auto& pRecord = result.first;
		return make_pair( make_pair(pRecord->first, pRecord->second), result.second );
	}

	template<typename TKey, typename TValue>
	template<class... Args >
	bool UnorderedMap<TKey,TValue>::Insert( function<void(TValue&)> afterInsert, Args&&... args )
	{
		std::unique_lock<shared_mutex> l{_mutex};
		auto result = base::emplace( args... );
		afterInsert( *result.first->second );
		return result.second;
	}
	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::TryEmplace( function<void(TValue&)> afterInsert, const TKey& key )
	{
		auto iteratorInserted = base::try_emplace( key );
		afterInsert( iteratorInserted.first->second );
		return iteratorInserted.second;
	}

	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::IfNone( function<bool(const TKey&,const TValue&)> ifFunction, function<void()> func )
	{
		auto pValue = std::find_if( base::begin(), base::end(), [&ifFunction](const std::pair<TKey,sp<TValue>>& keyValue){ return ifFunction(keyValue.first, *keyValue.second);} );
		bool isNone = pValue==base::end();
		if( isNone )
			func();
		return isNone;
	}
	template<typename TKey, typename TValue>
	sp<TValue> UnorderedMap<TKey,TValue>::Find( const TKey& key )Ι
	{
 		shared_lock<shared_mutex> l(_mutex);
		const auto pItem = base::find( key );
		return pItem==base::end() ? nullptr : pItem->second;
	}
	template<typename TKey, typename TValue>
	sp<TValue> UnorderedMap<TKey,TValue>::FindFirst( function<bool(const TValue&)> where )
	{
 		shared_lock<shared_mutex> l(_mutex);
		sp<TValue> pFound;
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
	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::ForFirst( function<bool(const TKey&, TValue&)> func )
	{
 		std::unique_lock<shared_mutex> l(_mutex);
		bool found = false;
		for( const auto& [key, pValue] : *this )
		{
			found = func( key, *pValue );
			if( found )
				break;
		}
		return found;
	}
#pragma region ForEach
	template<typename TKey, typename TValue>
	uint UnorderedMap<TKey,TValue>::ForEach( function<void(const TKey&, const TValue&)> fncn )const
	{
 		shared_lock<shared_mutex> l(_mutex);
		const auto count = size();
		for( const auto& [id, pValue] : *this )
			fncn( id, *pValue );
		return count;
	}
	template<typename TKey, typename TValue>
	uint UnorderedMap<TKey,TValue>::ForEach( function<void(const TKey&, TValue&)> fncn )
	{
 		unique_lock<shared_mutex> l( _mutex );
		const auto count = base::size();
		for( const auto& [id, pValue] : *this )
			fncn( id, *pValue );
		return count;
	}
#pragma endregion
	template<typename TKey, typename TValue>
	sp<std::forward_list<sp<TValue>>> UnorderedMap<TKey,TValue>::Values()const
	{
 		shared_lock<shared_mutex> l(_mutex);
		auto values = make_shared<std::forward_list<sp<TValue>>>();
		for( typename base::const_iterator ppValue = base::begin();  ppValue != base::end(); ++ppValue )
			values->push_front( ppValue->second );
		return values;
	}

	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::Set( const TKey& key, sp<TValue> pValue )ι
	{
		unique_lock<shared_mutex> l( _mutex );
		auto result = base::emplace( key, pValue );
		if( !result.second )
			result.first->second = pValue;
		return result.second;
	}
}
#undef LOCK
#undef var
#endif