#pragma once
#include <set>
#include <unordered_map>
#include <forward_list>
#include <shared_mutex>
#include <functional>
#include <jde/TypeDefs.h>
//#include "../SmartPointer.h"

namespace Jde
{
	using std::unordered_map;
	using std::set;
	using std::shared_lock;
	using std::unique_lock;
	using std::function;
namespace Collections
{
	template<typename TKey, typename TValue>
	class UnorderedMap : std::unordered_map<TKey,std::shared_ptr<TValue>>
	{
		typedef std::unordered_map<TKey,std::shared_ptr<TValue>> BaseClass;
	public:
		UnorderedMap( )=default;
		UnorderedMap( const UnorderedMap& copy );
		bool erase( const TKey& item )noexcept;
		bool eraseIf( const TKey& item, function<bool(const TValue&)> func )noexcept;
		uint eraseIf( function<bool(const TValue&)> func )noexcept;
		void clear()noexcept{ unique_lock<std::shared_mutex> l(_mutex); BaseClass::clear(); }
		uint size()const noexcept;
		//shared_ptr<TValue> FindNC( const TKey& key ){ return const_cast<shared_ptr<TValue>>( Find(key) ); }
		bool IfNone( function<bool(const TKey&,const TValue&)> ifFunction, function<void()> function );
		sp<TValue> Find( const TKey& key )const noexcept;
		sp<TValue> FindFirst( std::function<bool(const TValue&)> where );
		bool ForFirst( std::function<bool(const TKey&, TValue&)> func );
		uint ForEach( std::function<void(const TKey&, const TValue&)>& )const;
		uint ForEach( std::function<void(const TKey&, TValue&)>& );
		template<class... Args >
		std::pair<std::pair<TKey,sp<TValue>>,bool> emplace( Args&&... args );
		template<class... Args >
		bool Insert( function<void(TValue&)> afterInsert, Args&&... args );
		bool TryEmplace( function<void(TValue&)> afterInsert, const TKey& key );
		void Update( const TKey& key, function<void(TValue&)>& func );
		void Where( const TKey& key, function<void(const TValue&)> func );
		template<class... Args >
		void AddOrUpdate( TKey key, function<sp<TValue>()> add, function<void(TValue&)> update );
		std::shared_ptr<std::forward_list<std::shared_ptr<TValue>>> Values()const;
		//bool Set( const TKey& key, const TValue& value )noexcept;
		bool Set( const TKey& key, sp<TValue> pValue )noexcept;
		std::unique_lock<std::shared_mutex> Lock()noexcept{ return std::unique_lock<std::shared_mutex>{_mutex}; }
	private:
		mutable std::shared_mutex _mutex;
	};

	template<class TKey, class TValue> using UnorderedMapPtr = std::shared_ptr<UnorderedMap<TKey,TValue>>;

	template<typename TKey, typename TValue>
	UnorderedMap<TKey,TValue>::UnorderedMap( const UnorderedMap& copy )
	{
		std::unique_lock<std::shared_mutex> l( copy._mutex );
		for( const auto& [key,value] : copy )
			BaseClass::emplace( key, value );
	}

	template<typename TKey, typename TValue>
	void UnorderedMap<TKey,TValue>::Update( const TKey& key, std::function<void(TValue& value)>& func )
	{
 		shared_lock<std::shared_mutex> l(_mutex);
		auto pItem = BaseClass::find( key );
		if( pItem!=BaseClass::end() )
			func( *pItem->second );
	}

	template<typename TKey, typename TValue>
	void UnorderedMap<TKey,TValue>::Where( const TKey& key, function<void(const TValue&)> func )
	{
		shared_lock<std::shared_mutex> l(_mutex);
		auto pItem = BaseClass::find( key );
		if( pItem!=BaseClass::end() )
			func( *pItem->second );
	}

	template<typename TKey, typename TValue>
	template<class... Args >
	void UnorderedMap<TKey,TValue>::AddOrUpdate( TKey key, function<sp<TValue>()> add, function<void(TValue&)> update )
	{
 		unique_lock<std::shared_mutex> l(_mutex);
		auto pItem = BaseClass::find( key );
		if( pItem==BaseClass::end() )
			BaseClass::emplace( key, add() );
		else
			update( *pItem->second );
	}


	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::erase( const TKey& item )noexcept
	{
		unique_lock<std::shared_mutex> l( _mutex );
		return BaseClass::erase( item )>0;
	}
	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::eraseIf( const TKey& key, function<bool(const TValue&)> func )noexcept
	{
		unique_lock<std::shared_mutex> l( _mutex );
		auto pItem = BaseClass::find( key );
		bool erase = pItem!=BaseClass::end() && func( *pItem->second );
		if( erase )
			BaseClass::erase( pItem );
		return erase;
	}
	template<typename TKey, typename TValue>
	uint UnorderedMap<TKey,TValue>::eraseIf( function<bool(const TValue&)> func )noexcept
	{
		unique_lock<std::shared_mutex> l( _mutex );
		uint count = 0;
		for( auto pItem = BaseClass::begin(); pItem!=BaseClass::end(); )
		{
			if( func(*pItem->second) )
			{
				pItem = BaseClass::erase( pItem );
				++count;
			}
			else
				++pItem;
		}
		return count;
	}
	template<typename TKey, typename TValue>
	uint UnorderedMap<TKey,TValue>::size()const noexcept
	{
		shared_lock<std::shared_mutex> l(_mutex);
		return BaseClass::size();
	}

	template<typename TKey, typename TValue>
	template<class... Args >
	std::pair<std::pair<TKey,sp<TValue>>,bool> UnorderedMap<TKey,TValue>::emplace( Args&&... args )
	{
		std::unique_lock<std::shared_mutex> l{_mutex};
		auto result = BaseClass::emplace( args... );
		auto& pRecord = result.first;
		return make_pair( make_pair(pRecord->first, pRecord->second), result.second );
	}

	template<typename TKey, typename TValue>
	template<class... Args >
	bool UnorderedMap<TKey,TValue>::Insert( function<void(TValue&)> afterInsert, Args&&... args )
	{
		std::unique_lock<std::shared_mutex> l{_mutex};
		auto result = BaseClass::emplace( args... );
		afterInsert( *result.first->second );
		return result.second;
	}
	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::TryEmplace( function<void(TValue&)> afterInsert, const TKey& key )
	{
		auto iteratorInserted = BaseClass::try_emplace( key );
		afterInsert( iteratorInserted.first->second );
		return iteratorInserted.second;
	}

	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::IfNone( function<bool(const TKey&,const TValue&)> ifFunction, function<void()> func )
	{
		auto pValue = std::find_if( BaseClass::begin(), BaseClass::end(), [&ifFunction](const std::pair<TKey,sp<TValue>>& keyValue){ return ifFunction(keyValue.first, *keyValue.second);} );
		bool isNone = pValue==BaseClass::end();
		if( isNone )
			func();
		return isNone;
	}
	template<typename TKey, typename TValue>
	std::shared_ptr<TValue> UnorderedMap<TKey,TValue>::Find( const TKey& key )const noexcept
	{
 		shared_lock<std::shared_mutex> l(_mutex);
		const auto pItem = BaseClass::find( key );
		return pItem==BaseClass::end() ? nullptr : pItem->second;
	}
	template<typename TKey, typename TValue>
	shared_ptr<TValue> UnorderedMap<TKey,TValue>::FindFirst( std::function<bool(const TValue&)> where )
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
	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::ForFirst( std::function<bool(const TKey&, TValue&)> func )
	{
 		std::unique_lock<std::shared_mutex> l(_mutex);
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
	uint UnorderedMap<TKey,TValue>::ForEach( std::function<void(const TKey&, const TValue&)>& fncn )const
	{
 		shared_lock<std::shared_mutex> l(_mutex);
		const auto count = size();
		for( const auto& [id, pValue] : *this )
			fncn( id, *pValue );
		return count;
	}
	template<typename TKey, typename TValue>
	uint UnorderedMap<TKey,TValue>::ForEach( std::function<void(const TKey&, TValue&)>& fncn )
	{
 		unique_lock<std::shared_mutex> l( _mutex );
		const auto count = BaseClass::size();
		for( const auto& [id, pValue] : *this )
			fncn( id, *pValue );
		return count;
	}
#pragma endregion
	template<typename TKey, typename TValue>
	std::shared_ptr<std::forward_list<std::shared_ptr<TValue>>> UnorderedMap<TKey,TValue>::Values()const
	{
 		shared_lock<std::shared_mutex> l(_mutex);
		auto values = make_shared<std::forward_list<std::shared_ptr<TValue>>>();
		for( typename BaseClass::const_iterator ppValue = BaseClass::begin();  ppValue != BaseClass::end(); ++ppValue )
			values->push_front( ppValue->second );
		return values;
	}
/*	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::Set( const TKey& key, const TValue& value )noexcept
	{
		return Set()
		auto newValue = make_shared<TValue>( value );
	}*/
	template<typename TKey, typename TValue>
	bool UnorderedMap<TKey,TValue>::Set( const TKey& key, sp<TValue> pValue )noexcept
	{
		unique_lock<std::shared_mutex> l( _mutex );
		auto result = BaseClass::emplace( key, pValue );
		if( !result.second )
			result.first->second = pValue;
		return result.second;
	}

}}