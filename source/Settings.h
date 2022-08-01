#pragma once
#include <variant>
#include "DateTime.h"
#include <jde/Assert.h>
#include <jde/Exports.h>
#include <jde/Exception.h>
#include <jde/Str.h>
#include <jde/App.h>

#pragma warning(push)
#pragma warning( disable : 4715)
#include <nlohmann/json.hpp>
#pragma warning(pop)

#define var const auto
namespace Jde
{
	using nlohmann::json;
}
namespace Jde::Settings
{
	α Path()ι->fs::path;
	struct JsonNumber
	{
		using Variant=std::variant<double,_int,uint>;
		JsonNumber( json j )ε;
		JsonNumber( double v )ι:Value{v}{};
		JsonNumber( _int v )ι:Value{v}{};
		JsonNumber( uint v )ι:Value{v}{};

		Variant Value;
	};
	struct Γ Container
	{
		using Variant=std::variant<nullptr_t,string,JsonNumber>;
		Container( const json& json )ι;
		Container( path jsonFile, SRCE )ε;
		α TryMembers( sv path )ι->flat_map<string,Container>;
		α Have( sv path )ι->bool;
		α FindPath( sv path )Ι->optional<json>;
		Ŧ TryArray( sv path, vector<T> dflt )ι->vector<T>;
		Ŧ Map( sv path )ι->flat_map<string,T>;
		Variant operator[]( sv path )ε;

		α ForEach( sv path, function<void(sv, const nlohmann::json&)> f )ι->void;

		α SubContainer( sv entry )const ε->sp<Container>;
		α TrySubContainer( sv entry )Ι->optional<Container>;
		Ŧ Getɛ( sv path, SRCE )Ε->T;
		template<class T=string> auto Get( sv path )Ι->optional<T>;

		α& Json()ι{ /*ASSERT(_pJson);*/ return *_pJson; }
	private:
		up<json> _pJson;
	};

	Γ α Global()ι->Container&;

	α FileStem()ι->string;
	#define $ template<> Ξ
	$ Container::Getɛ<TimePoint>( sv path, const source_location& sl )Ε->TimePoint{ return DateTime{ Getɛ<string>(path, sl) }.GetTimePoint(); }
	$ Container::Getɛ<fs::path>( sv path, const source_location& )Ε->fs::path{ var p = Get<string>(path); return p.has_value() ? fs::path{*p} : fs::path{}; }

	Ŧ Container::Getɛ( sv path, const source_location& sl )Ε->T
	{
		auto p = Get<T>( path ); if( !p ) throw Exception{ sl, ELogLevel::Debug, "'{}' was not found in settings.", path };//mysql precludes using THROW_IF
		return *p;
	}

	$ Container::Get<Duration>( sv path )Ι->optional<Duration>
	{
		var strng = Get<string>( path );
		optional<std::chrono::system_clock::duration> result;
		if( strng.has_value() )
			Try( [strng, &result](){ result = Chrono::ToDuration(*strng);} );
		return  result;
	}
	$ Container::Get<ELogLevel>( sv path )Ι->optional<ELogLevel>
	{
		optional<ELogLevel> level;
		if( auto p = FindPath(path); p )
		{
			if( p->is_string() )
				level = Str::ToEnum<ELogLevel,array<sv,7>>( ELogLevelStrings, p->get<string>() );
			else if( p->is_number() && p->get<uint8>()<ELogLevelStrings.size() )
				level = (ELogLevel)p->get<int8>();
		}
		return level;
	}

	Ξ Container::TryMembers( sv path )ι->flat_map<string,Container>
	{
		flat_map<string,Container> members;
		auto j = FindPath( path );
		if( j && j->is_object() )
		{
			auto obj = j->get<json::object_t>();
			for( auto& [name,j2] : obj )
				members.emplace( name, Container{ j2 } );
		}
		return members;
	}

	$ Container::Get<fs::path>( sv path )Ι->optional<fs::path>
	{
		auto p = Get<string>( path );
		if( var i{p ? p->find("$(") : string::npos}; i!=string::npos && i<p->size()-3 )
		{
			var end = p->find( ')', i );
			var env = OSApp::EnvironmentVariable( p->substr(i+2, end-i-2), SRCE_CUR );
			p = p->substr( 0, i )+env+p->substr( end+1 );
		}
		return p ? optional<fs::path>(*p) : nullopt;
	}

	Ŧ Container::Get( sv path )Ι->optional<T>
	{
		auto p = FindPath( path );
		try
		{
			return p ? optional<T>{ p->get<T>() } : nullopt;
		}
		catch( const nlohmann::detail::type_error& e )
		{
			DBG_ONCE( "({}) - {}", path, e.what() );
			return nullopt;
		}
	}

	$ Container::TryArray<Container>( sv path, vector<Container> dflt )ι->vector<Container>
	{
		vector<Container> values;
		if( auto p = FindPath(path); p )
		{
			for( auto& i : p->items() )
				values.emplace_back( i.value() );
		}
		else
			values = move( dflt );

		return values;
	}
	//Γ α Set( sv path, path value )ι->void;

	α Save( const json& j, sv what, SRCE )ε->void;
	α Set( sv what, const Container::Variant& v, bool save=true, SRCE )ε->void;

	Ŧ TryArray( sv path, vector<T> dflt={} )ι{ return Global().TryArray<T>(path, dflt); }

	Ŧ Container::TryArray( sv path, vector<T> dflt )ι->vector<T>
	{
		vector<T> values;
		if( auto p = FindPath(path); p && p->is_array() )
		{
			for( var& i : *p )
				values.push_back( i.get<T>() );
		}
		else
			values = move( dflt );
		return values;
	}

	Ŧ Container::Map( sv path )ι->flat_map<string,T>
	{
		auto pItem = _pJson->find( path );
		flat_map<string,T> values;
		if( pItem!=_pJson->end() )
		{
			for( var& [key,value] : pItem->items() )
				values.emplace( key, value );
		}
		return values;
	}

	Ξ Container::ForEach( sv path, function<void(sv, const nlohmann::json&)> f )ι->void
	{

		if( auto p = FindPath( path ); p && p->is_object() )
		{
			for( auto&& i : p->items() )
				f( i.key(), i.value() );
		}
	}


	Ŧ Getɛ( sv path, SRCE )ε{ return Global().Getɛ<T>( path, sl ); }
	template<class T=string> auto Get( sv path )ι->optional<T>{ return Global().Get<T>( path ); }
	$ Get<Duration>( sv path )ι->optional<Duration>{ return Global().Get<Duration>( path ); }
	Ξ TryMembers( sv path )ι->flat_map<string,Container>{ return Global().TryMembers( path ); }
	$ Get<ELogLevel>( sv path )ι->optional<ELogLevel>{ return Global().Get<ELogLevel>( path ); }
	Ξ ForEach( sv path, function<void(sv, const nlohmann::json& v)> f )ι->void{ return Global().ForEach(path, f); }

	Ξ Env( sv path, SRCE )ι->optional<string>
	{
		auto p = Global().Get( path );
		if( p && p->starts_with("$(") && p->size()>3 )
		{
			p = OSApp::EnvironmentVariable( p->substr(2, p->size()-3), sl );
			if( p->size()==0 )
				p = nullopt;
		}
		return p;
	}

	Ξ Envɛ( sv path )ε->string
	{
		auto p = Global().Get( path );
		if( p && p->starts_with("$(") && p->size()>3 )
			p = OSApp::EnvironmentVariable( p->substr(2, p->size()-3) );
		if( !p ) throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Debug, "{} not found", path }; //can't use THROW_IF
		return *p;
	}

	Τ struct Item
	{
		Item( sv path, T dflt ):
			Value{ Get<T>(path).value_or(dflt) }
		{}
		operator T(){return Value;}
		const T Value;
	};

	Ŧ TryGetSubcontainer( sv container, sv path )ι->optional<T>
	{
		optional<T> v;
		if( auto pSub=Global().TrySubContainer( container ); pSub )
			v = pSub->Get<T>( path );
		return v;
	}

	$ TryGetSubcontainer<Container>( sv container, sv path )ι->optional<Container>
	{
		optional<Container> v;
			if( auto pSub=Global().TrySubContainer( container ); pSub )
				v = pSub->TrySubContainer( path );
		return v;
	}
}
#undef var
#undef $