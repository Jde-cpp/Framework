#pragma once

#define SLOCK shared_lock<shared_mutex> l( Mutex );
#define LOCK unique_lock<shared_mutex> l( Mutex );
namespace Jde
{
	template<typename T>
	class Vector : private vector<T>
	{
		typedef vector<T> base;
	public:
		Vector()noexcept:base{}{}
		Vector( uint size )noexcept:base{}{ base::reserve(size); }
		void clear()noexcept{ LOCK base::clear(); }
		void push_back( const T& val )noexcept{ LOCK base::push_back( val ); }
		void push_back( T&& val )noexcept{ LOCK base::push_back( move(val) ); }
		uint size()const noexcept{ SLOCK return size( l ); }
		uint size( shared_lock<shared_mutex>& l )const noexcept{ return base::size(); }
		typename base::const_iterator begin( shared_lock<shared_mutex>& l )const noexcept{ return base::begin(); }
		typename base::const_iterator end( shared_lock<shared_mutex>& l )const noexcept{ return base::end(); }
		mutable std::shared_mutex Mutex;
	};
}
#undef SLOCK
#undef LOCK