#pragma once

namespace Jde
{
	//using std::unordered_map;
	//using std::set;
	//using std::shared_lock;
	//using std::unique_lock;
	//using std::function;
	template<typename TKey, typename TValue>
	class UnorderedMapValue : std::unordered_map<TKey,TValue>
	{
		typedef std::unordered_map<TKey,TValue> base;
	public:
		UnorderedMapValue( )=default;
		UnorderedMapValue( const UnorderedMapValue& copy )=delete;
		void erase( const TKey& item )ι{ unique_lock<std::shared_mutex> l( _mutex ); base::erase( item ); }
		const std::unordered_map<TKey,TValue> Find( std::function<bool(const TValue&)> func )const;
		optional<TValue> Find( const TKey& key )ι;
		uint ForEach( std::function<void(const TKey&, const TValue&)> fncn )Ι;
		bool Has( const TKey& key )Ι;
		ψ emplace( Args&&... args )ι->bool;
		void Replace( const TKey& id, const TValue& value )ι;
		bool MoveIn( const TKey& id, TValue v )ι{ return base::emplace(id,move(v)).second; }
		optional<TValue> MoveOut( const TKey& item )ι;
		α size()ι->uint{ return base::size(); }
	private:
		mutable std::shared_mutex _mutex;
	};
	template<typename TKey, typename TValue>
	ψ UnorderedMapValue<TKey,TValue>::emplace( Args&&... args )ι->bool
	{
		std::unique_lock<std::shared_mutex> l{ _mutex };
		const auto result = base::emplace( args... );
		return result.second;
	}
	template<typename TKey, typename TValue>
	const std::unordered_map<TKey,TValue> UnorderedMapValue<TKey,TValue>::Find( std::function<bool(const TValue&)> func )const
	{
		std::unordered_map<TKey,TValue> results;
		shared_lock<std::shared_mutex> l( _mutex );
		for( const auto& keyValue : *this )
		{
			if( func(keyValue.second) )
				results.emplace(keyValue.first, keyValue.second );
		}
		return results;
	}

	template<typename TKey, typename TValue>
	optional<TValue> UnorderedMapValue<TKey,TValue>::Find( const TKey& key )ι//TODO forward args like try_emplace
	{
		shared_lock<std::shared_mutex> l(_mutex);
		const auto pKeyValue = base::find( key );
		return pKeyValue==base::end() ? std::nullopt : optional<TValue>{pKeyValue->second};
	}

	template<typename TKey, typename TValue>
	void UnorderedMapValue<TKey,TValue>::Replace( const TKey& id, const TValue& value )ι
	{
		unique_lock<std::shared_mutex> l(_mutex);
		if( base::find(id)!=base::end() )
			base::at(id)=value;
		else
			base::emplace( id, value );
	}
	template<typename TKey, typename TValue>
	bool UnorderedMapValue<TKey,TValue>::Has( const TKey& id )Ι
	{
		shared_lock<std::shared_mutex> l(_mutex);
		return base::find(id)!=base::end();
	}

	template<typename TKey, typename TValue>
	uint UnorderedMapValue<TKey,TValue>::ForEach( std::function<void(const TKey&, const TValue&)> fncn )Ι
	{
 		shared_lock<std::shared_mutex> l(_mutex);
		const auto count = base::size();
		for( const auto& [id, value] : *this )
			fncn( id, value );
		return count;
	}
	template<typename TKey, typename TValue>
	optional<TValue> UnorderedMapValue<TKey,TValue>::MoveOut( const TKey& id )ι
	{
		optional<TValue> v;
		unique_lock<std::shared_mutex> l( _mutex );
		if( auto p=base::find(id); p!=base::end() )
		{
			v = move( p->second );
			base::erase( p );
		}
		return v;
	}
}