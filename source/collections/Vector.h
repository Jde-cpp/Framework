#pragma once

#define SLOCK
#define LOCK
namespace Jde{
	template<typename T>
	struct Vector : private vector<T>{
		using base=vector<T>;
		Vector()ι:base{}{}
		Vector( uint size )ι:base{}{ base::reserve(size); }

		α begin( sl& l )Ι->typename base::const_iterator{ return base::begin(); }
		α end( sl& l )Ι->typename base::const_iterator{ return base::end(); }
		α begin( ul& l )ι->typename base::iterator{ return base::begin(); }
		α end( ul& l )ι->typename base::iterator{ return base::end(); }

		α clear()ι{ ul _( Mutex ); base::clear(); }
		α find( const T& x )ι->optional<T>{ sl l( Mutex ); auto p = std::ranges::find(Base(), x); return p==end(l) ? nullopt : optional<T>{ *p }; }
		α erase( const T& x )ι->bool{ ul l( Mutex ); auto p = std::ranges::find(Base(), x); bool found = p!=end(l); base::erase(p); return found; }
		α	erase( function<void(const T& p)> before )ι->void;
		α	erase_if( function<bool(const T& p)> test )ι->void;

		α push_back( const T& val )ι{ ul _( Mutex ); base::push_back( val ); }
		α push_back( T&& val )ι{ ul _( Mutex ); base::push_back( move(val) ); }
		α size()Ι->uint{ sl l( Mutex ); return size( l ); }
		α size( sl& l )Ι->uint{ return base::size(); }
		α visit( function<void(const T& p)> f )ι->void;

		mutable std::shared_mutex Mutex;
		private:
		α Base()ι->vector<T>&{ return (vector<T>&)*this; }
	};

	Ŧ	Vector<T>::erase( function<void(const T& p)> before )ι->void{
		ul _( Mutex );
		for( auto p = base::begin(); p!=base::end(); p=base::erase(p) )
			before( *p );
	}
	Ŧ	Vector<T>::erase_if( function<bool(const T& p)> test )ι->void{
		ul _( Mutex );
		for( auto p=base::begin(); p!=base::end(); p = test( *p ) ? base::erase( p ) : std::next( p ) );
	}
	Ŧ	Vector<T>::visit( function<void(const T& p)> f )ι->void{
		ul _( Mutex );
		for_each( Base(), f );
	}
}
#undef LOCK