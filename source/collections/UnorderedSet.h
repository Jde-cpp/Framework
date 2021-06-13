#pragma once
#include <unordered_set>
#include <shared_mutex>

namespace Jde
{
	using std::unordered_set;
	using std::function;
	template<typename TKey>
	class UnorderedSet : unordered_set<TKey>
	{
		typedef unordered_set<TKey> base;
	public:
		UnorderedSet()=default;
		UnorderedSet( const unordered_set<TKey>& values )noexcept:base( values ){};
		UnorderedSet( const UnorderedSet<TKey>& values )noexcept;
		//UnorderedSet(const UnorderedSet&)=delete;
		//UnorderedSet(const UnorderedSet&)=delete;
		unordered_set<TKey> ToSet()const noexcept;
		uint erase(const TKey& item)noexcept;
		size_t EraseIf( function<bool(const TKey&)> func )noexcept;
		bool IfEmpty( function<void()> func )noexcept;
		uint size()const noexcept;
		bool contains( const TKey& x )const noexcept{ std::unique_lock<std::shared_mutex> l(_mutex);  return base::contains( x ); }
		void clear()noexcept{ std::unique_lock<std::shared_mutex> l(_mutex);  return base::clear(); }
		template<class... Args >
		bool emplace( Args&&... args )noexcept;
		uint ForEach( std::function<void(const TKey& key)> func )const noexcept(false);
		bool MoveIn( TKey&& id )noexcept{ unique_lock<shared_mutex> l(_mutex); return base::emplace( move(id) ).second; }
	private:
		mutable std::shared_mutex _mutex;
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
		std::unique_lock<std::shared_mutex> l( _mutex );
		for( const auto& value : *this )
			copy.emplace( value );
		return copy;
	}
	template<typename TKey>
	uint UnorderedSet<TKey>::erase(const TKey& item)noexcept
	{
		std::unique_lock<std::shared_mutex> l( _mutex );
		return base::erase( item );//assume compare doesn't throw
	}
	template<typename TKey>
	size_t UnorderedSet<TKey>::EraseIf( function<bool(const TKey&)> func )noexcept
	{
		std::unique_lock<std::shared_mutex> l( _mutex );
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
		std::unique_lock<std::shared_mutex> l( _mutex );
		bool isEmpty = base::size()==0;
		if( isEmpty )
			func();
		return isEmpty;
	}
	template<typename TKey>
	uint UnorderedSet<TKey>::size()const noexcept
	{
		std::shared_lock<std::shared_mutex> l(_mutex);
		return base::size();
	}

	template<typename TKey>
	template<class... Args >
	bool UnorderedSet<TKey>::emplace( Args&&... args )noexcept
	{
		std::unique_lock<std::shared_mutex> l(_mutex);
		const auto result = base::emplace( args... );
		return result.second;
	}
	template<typename TKey>
	uint UnorderedSet<TKey>::ForEach( std::function<void(const TKey& key)> func )const noexcept(false)
	{
		std::shared_lock<std::shared_mutex> l(_mutex);
		uint count = size();
		for( const auto& value : *this )
			func( value );
		return count;
	}
}