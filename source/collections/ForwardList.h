#pragma once
#include <forward_list>
#include <shared_mutex>
//#include "../SmartPointer.h"

namespace Jde
{
	using std::shared_lock;
	using std::unique_lock;
	using std::forward_list;
namespace Collections
{
	template<typename TValue>
	class ForwardListValue : forward_list<TValue>
	{
		typedef forward_list<TValue> BaseClass;
	public:
		void push_front( const TValue& val ); 
		void for_each( std::function<void(const TValue&)>& func );
	private:
		mutable std::shared_mutex _mutex;
	};

	template<typename TValue>
	void ForwardListValue<TValue>::push_front( const TValue& value )
	{
		unique_lock<std::shared_mutex> l( _mutex ); 
		BaseClass::push_front( value );
	}

	template<typename TValue>
	void ForwardListValue<TValue>::for_each( std::function<void(const TValue&)>& func )
	{
		shared_lock<std::shared_mutex> l( _mutex ); 
		std::for_each( BaseClass::begin(), BaseClass::end(), [func](const TValue& value){func(value);} );
	}
	
}}
