#pragma once

#define SLOCK shared_lock<shared_mutex> l( Mutex );
#define LOCK unique_lock<shared_mutex> l( Mutex );
namespace Jde{
	template<typename T>
	struct Vector : private vector<T>{
		using base=vector<T>;
		Vector()ι:base{}{}
		Vector( uint size )ι:base{}{ base::reserve(size); }
		void clear()ι{ LOCK base::clear(); }
		void push_back( const T& val )ι{ LOCK base::push_back( val ); }
		void push_back( T&& val )ι{ LOCK base::push_back( move(val) ); }
		uint size()Ι{ SLOCK return size( l ); }
		uint size( shared_lock<shared_mutex>& l )Ι{ return base::size(); }
		typename base::const_iterator begin( shared_lock<shared_mutex>& l )Ι{ return base::begin(); }
		typename base::const_iterator end( shared_lock<shared_mutex>& l )Ι{ return base::end(); }
		mutable std::shared_mutex Mutex;
	};
}
#undef SLOCK
#undef LOCK