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
		typedef std::unordered_map<TKey,TValue> BaseClass;
	public:
		UnorderedMapValue( )=default;
		UnorderedMapValue( const UnorderedMapValue& copy )=delete;
		void erase( const TKey& item )noexcept{ unique_lock<std::shared_mutex> l( _mutex ); BaseClass::erase( item ); }
		const unordered_map<TKey,TValue> Find( std::function<bool(const TValue&)> func )const;
		TValue Find( const TKey& key, const TValue& dflt )noexcept;
		bool Has( const TKey& key )noexcept;
		template<class... Args >
		bool emplace( Args&&... args )noexcept;
		void Replace( const TKey& id, const TValue& value )noexcept;
	private:
		mutable std::shared_mutex _mutex;	
	};
	template<typename TKey, typename TValue>
	template<class... Args >
	bool UnorderedMapValue<TKey,TValue>::emplace( Args&&... args )noexcept
	{
		std::unique_lock<std::shared_mutex> l;
		const auto result = BaseClass::emplace( args... );
		return result.second;
	}
	template<typename TKey, typename TValue>
	const unordered_map<TKey,TValue> UnorderedMapValue<TKey,TValue>::Find( std::function<bool(const TValue&)> func )const
	{
		unordered_map<TKey,TValue> results;
		shared_lock<std::shared_mutex> l( _mutex ); 
		for( const auto& keyValue : *this )
		{
			if( func(keyValue.second) )
				results.emplace(keyValue.first, keyValue.second );
		}
		return results;
	}

	template<typename TKey, typename TValue>
	TValue UnorderedMapValue<TKey,TValue>::Find( const TKey& key, const TValue& dflt )noexcept
	{
		shared_lock<std::shared_mutex> l(_mutex);
		const auto pKeyValue = BaseClass::find( key );
		return pKeyValue==BaseClass::end() ? dflt : pKeyValue->second;
	}
	
	template<typename TKey, typename TValue>
	void UnorderedMapValue<TKey,TValue>::Replace( const TKey& id, const TValue& value )noexcept
	{
		unique_lock<std::shared_mutex> l(_mutex);
		if( BaseClass::find(id)!=BaseClass::end() )
			BaseClass::at(id)=value;
		else
			BaseClass::emplace( id, value );
	}
	template<typename TKey, typename TValue>
	bool UnorderedMapValue<TKey,TValue>::Has( const TKey& id )noexcept
	{
		shared_lock<std::shared_mutex> l(_mutex);
		return BaseClass::find(id)!=BaseClass::end();
	}
}