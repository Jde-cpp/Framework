#pragma once

namespace Jde::Collections
{
	template<typename TValue>
	class List : list<shared_ptr<TValue>>
	{
		typedef list<shared_ptr<TValue>> BaseClass;
	public:
		bool eraseFirst( function<bool(const TValue&)>& );
		void push_back( sp<TValue>& val ); 
		void for_each( std::function<void(TValue&)>& func );
		void for_each( std::function<void(const TValue&)>& func )const;
		bool ForFirst( std::function<bool(const TValue&)>& func )const;
		bool ForFirst( std::function<bool(TValue&)>& func );
		shared_ptr<TValue> front();
		bool Has( function<bool(const TValue&)> ifFunction )const;
		sp<TValue> Find( function<bool(const TValue&)> ifFunction )const;
		uint size()const noexcept{ shared_lock<std::shared_mutex> l( _mutex ); return BaseClass::size(); }
	private:
		mutable std::shared_mutex _mutex;
	};

	template<typename TValue>
	void List<TValue>::push_back( sp<TValue>& value )
	{
		unique_lock<std::shared_mutex> l( _mutex ); 
		BaseClass::push_back( value );
	}

	template<typename TValue>
	void List<TValue>::for_each( std::function<void(TValue&)>& func )
	{
		shared_lock<std::shared_mutex> l( _mutex ); 
		std::for_each( BaseClass::begin(), BaseClass::end(), [&func](sp<TValue>& pValue){ func(*pValue); } );
	}
	template<typename TValue>
	void List<TValue>::for_each( std::function<void(const TValue&)>& func )const
	{
		shared_lock<std::shared_mutex> l( _mutex ); 
		std::for_each( BaseClass::begin(), BaseClass::end(), [&func](const TValue& value){func(value);} );
	}
	template<typename TValue>
	bool List<TValue>::ForFirst( std::function<bool(const TValue&)>& func )const
	{
		unique_lock<std::shared_mutex> l( _mutex );
		bool found = false;
		for( auto pValue : *this )
		{
			found = func( *pValue );
			if( found )
				break;
		}
		return found;
	}
	template<typename TValue>
	bool List<TValue>::ForFirst( std::function<bool(TValue&)>& func )
	{
		unique_lock<std::shared_mutex> l( _mutex );
		bool found = false;
		for( auto pValue : *this )
		{
			found = func( *pValue );
			if( found )
				break;
		}
		return found;
	}
	template<typename TValue>
	bool List<TValue>::Has( function<bool(const TValue&)> ifFunction )const
	{
		shared_lock<std::shared_mutex> l( _mutex );
		return std::find_if( BaseClass::begin(), BaseClass::end(), [&ifFunction](const shared_ptr<TValue>& pValue){return ifFunction(*pValue);} )!=BaseClass::end();
	}
	
	template<typename TValue>
	sp<TValue> List<TValue>::Find( function<bool(const TValue&)> ifFunction )const
	{
		shared_lock<std::shared_mutex> l( _mutex );
		auto ppValue = std::find_if( BaseClass::begin(), BaseClass::end(), [&ifFunction](const shared_ptr<TValue>& pValue){return ifFunction(*pValue);} );
		return ppValue==BaseClass::end() ? sp<TValue>{nullptr} : *ppValue;
	}

	template<typename TValue>
	bool List<TValue>::eraseFirst( function<bool(const TValue&)>& func )
	{
		unique_lock<std::shared_mutex> l( _mutex );
		auto ppValue = BaseClass::begin();
		for( ; ppValue!=BaseClass::end(); ++ppValue )
		{
			if( func(*(*ppValue)) )
			{
				BaseClass::erase( ppValue );
				break;
			}
		}
		return ppValue!=BaseClass::end();
	}
	template<typename TValue>
	shared_ptr<TValue> List<TValue>::front()
	{
		shared_lock<std::shared_mutex> l( _mutex );
		return size() ? *BaseClass::begin() : shared_ptr<TValue>{nullptr};
	}
}
