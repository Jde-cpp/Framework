#pragma once
#include <unordered_set>
#include <shared_mutex>

namespace Jde
{
	using std::unordered_set;
	using std::function;
	#define LOCK unique_lock<shared_mutex> l(_mutex)
	#define var const auto
	template<typename TKey>
	struct UnorderedSet : private unordered_set<TKey>
	{
		typedef unordered_set<TKey> base;
		UnorderedSet()=default;
		UnorderedSet( const unordered_set<TKey>& values )noexcept:base( values ){};
		UnorderedSet( const UnorderedSet<TKey>& values )noexcept;
		UnorderedSet& operator=( base&& x ){ LOCK; base::clear(); for( var& i : x)base::emplace(i); x.clear(); return *this; }
		unordered_set<TKey> ToSet()const noexcept;
		uint erase(const TKey& item)noexcept;
		size_t EraseIf( function<bool(const TKey&)> func )noexcept;
		bool IfEmpty( function<void()> func )noexcept;
		uint size()const noexcept;
		bool contains( const TKey& x )const noexcept{ LOCK;  return base::contains( x ); }
		void clear()noexcept{ LOCK;  return base::clear(); }
		template<class... Args >
		bool emplace( Args&&... args )noexcept;
		uint ForEach( std::function<void(const TKey& key)> func )const noexcept(false);
		bool MoveIn( TKey&& id )noexcept{ LOCK; return base::emplace( move(id) ).second; }
	private:
		mutable shared_mutex _mutex;
	};
	template<typename TKey>
	UnorderedSet<TKey>::UnorderedSet( const UnorderedSet<TKey>& values )noexcept
	{
		values.ForEach( [&](auto key)
		{
			base::emplace( key );
		});
	}
	template<typename TKey>
	unordered_set<TKey> UnorderedSet<TKey>::ToSet()const noexcept
	{
		unordered_set<TKey> copy;
		LOCK;
		for( const auto& value : *this )
			copy.emplace( value );
		return copy;
	}
	template<typename TKey>
	uint UnorderedSet<TKey>::erase(const TKey& item)noexcept
	{
		LOCK;
		return base::erase( item );//assume compare doesn't throw
	}
	template<typename TKey>
	size_t UnorderedSet<TKey>::EraseIf( function<bool(const TKey&)> func )noexcept
	{
		LOCK;
		size_t count = 0;
		for( auto pKey = base::begin(); pKey!=base::end(); )
		{
			if( func(*pKey) )
			{
				pKey = base::erase( pKey );
				++count;
			}
			else
				++pKey;
		}
		return count;
	}
	template<typename TKey>
	bool UnorderedSet<TKey>::IfEmpty( function<void()> func )noexcept
	{
		LOCK;
		bool isEmpty = base::size()==0;
		if( isEmpty )
			func();
		return isEmpty;
	}
	template<typename TKey>
	uint UnorderedSet<TKey>::size()const noexcept
	{
		std::shared_lock<shared_mutex> l(_mutex);
		return base::size();
	}

	template<typename TKey>
	template<class... Args >
	bool UnorderedSet<TKey>::emplace( Args&&... args )noexcept
	{
		LOCK;
		const auto result = base::emplace( args... );
		return result.second;
	}
	template<typename TKey>
	uint UnorderedSet<TKey>::ForEach( std::function<void(const TKey& key)> func )const noexcept(false)
	{
		std::shared_lock<shared_mutex> l(_mutex);
		uint count = size();
		for( const auto& value : *this )
			func( value );
		return count;
	}
#undef var
#undef LOCK
}