﻿#pragma once
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
	α Path()noexcept->fs::path;
	struct JsonNumber
	{
		using Variant=std::variant<double,_int,uint>;
		JsonNumber( json j )noexcept(false);
		JsonNumber( double v )noexcept:Value{v}{};
		JsonNumber( _int v )noexcept:Value{v}{};
		JsonNumber( uint v )noexcept:Value{v}{};

		Variant Value;
	};
	struct Γ Container
	{
		using Variant=std::variant<nullptr_t,string,JsonNumber>;
		Container( const json& json )noexcept;
		Container( path jsonFile, SRCE )noexcept(false);
		α TryMembers( sv path )noexcept->flat_map<string,Container>;
		α Have( sv path )noexcept->bool;
		α FindPath( sv path )const noexcept->optional<json>;
		ⓣ TryArray( sv path )noexcept->vector<T>;
		ⓣ Map( sv path )noexcept->flat_map<string,T>;
		Variant operator[]( sv path )noexcept(false);

		α ForEach( sv path, function<void(sv, const nlohmann::json&)> f )noexcept->void;

		α SubContainer( sv entry )const noexcept(false)->sp<Container>;
		α TrySubContainer( sv entry )const noexcept->optional<Container>;
		ⓣ Getɛ( sv path, SRCE )const noexcept(false)->T;
		template<class T=string> auto Get( sv path )const noexcept->optional<T>;

		α& Json()noexcept{ /*ASSERT(_pJson);*/ return *_pJson; }
	private:
		up<json> _pJson;
	};

	Γ α Global()noexcept->Container&;

	α FileStem()noexcept->string;
	#define $ template<> Ξ
	$ Container::Getɛ<TimePoint>( sv path, const source_location& sl )const noexcept(false)->TimePoint{ return DateTime{ Getɛ<string>(path, sl) }.GetTimePoint(); }
	$ Container::Getɛ<fs::path>( sv path, const source_location& )const noexcept(false)->fs::path{ var p = Get<string>(path); return p.has_value() ? fs::path{*p} : fs::path{}; }

	ⓣ Container::Getɛ( sv path, const source_location& sl )const noexcept(false)->T
	{
		auto p = Get<T>( path ); if( !p ) throw Exception{ sl, ELogLevel::Debug, "'{}' was not found in settings.", path };//mysql precludes using THROW_IF
		return *p;
	}

	$ Container::Get<Duration>( sv path )const noexcept->optional<Duration>
	{
		var strng = Get<string>( path );
		optional<std::chrono::system_clock::duration> result;
		if( strng.has_value() )
			Try( [strng, &result](){ result = Chrono::ToDuration(*strng);} );
		return  result;
	}
	$ Container::Get<ELogLevel>( sv path )const noexcept->optional<ELogLevel>
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

	Ξ Container::TryMembers( sv path )noexcept->flat_map<string,Container>
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

	$ Container::Get<fs::path>( sv path )const noexcept->optional<fs::path>
	{
		var p = Get<string>( path );
		return p ? optional<fs::path>(*p) : nullopt;
	}

	ⓣ Container::Get( sv path )const noexcept->optional<T>
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

	$ Container::TryArray<Container>( sv path )noexcept->vector<Container>
	{
		vector<Container> values;
		if( auto p = FindPath(path); p )
		{
			for( auto& i : p->items() )
				values.emplace_back( i.value() );
		}
		return values;
	}
	//Γ α Set( sv path, path value )noexcept->void;

	α Save( const json& j, sv what, SRCE )noexcept(false)->void;
	α Set( sv what, const Container::Variant& v, bool save=true, SRCE )noexcept(false)->void;

	ⓣ TryArray( sv path )noexcept{ return Global().TryArray<T>(path); }

	ⓣ Container::TryArray( sv path )noexcept->vector<T>
	{
		vector<T> values;
		if( auto p = FindPath(path); p && p->is_array() )
		{
			for( var& i : *p )
				values.push_back( i.get<T>() );
		}
		return values;
	}

	ⓣ Container::Map( sv path )noexcept->flat_map<string,T>
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

	Ξ Container::ForEach( sv path, function<void(sv, const nlohmann::json&)> f )noexcept->void
	{

		if( auto p = FindPath( path ); p && p->is_object() )
		{
			for( auto&& i : p->items() )
				f( i.key(), i.value() );
		}
	}


	ⓣ Getɛ( sv path, SRCE )noexcept(false){ return Global().Getɛ<T>( path, sl ); }
	template<class T=string> auto Get( sv path )noexcept->optional<T>{ return Global().Get<T>( path ); }
	$ Get<Duration>( sv path )noexcept->optional<Duration>{ return Global().Get<Duration>( path ); }
	Ξ TryMembers( sv path )noexcept->flat_map<string,Container>{ return Global().TryMembers( path ); }
	$ Get<ELogLevel>( sv path )noexcept->optional<ELogLevel>{ return Global().Get<ELogLevel>( path ); }
	Ξ ForEach( sv path, function<void(sv, const nlohmann::json& v)> f )noexcept->void{ return Global().ForEach(path, f); }

	Ξ Env( sv path )noexcept->optional<string>
	{
		auto p = Global().Get( path );
		if( p && p->starts_with("$(") && p->size()>3 )
			p = OSApp::EnvironmentVariable( p->substr(2, p->size()-3) );
		return p;
	}

	Ξ Envɛ( sv path )noexcept(false)->string
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

	ⓣ TryGetSubcontainer( sv container, sv path )noexcept->optional<T>
	{
		optional<T> v;
		if( auto pSub=Global().TrySubContainer( container ); pSub )
			v = pSub->Get<T>( path );
		return v;
	}

	$ TryGetSubcontainer<Container>( sv container, sv path )noexcept->optional<Container>
	{
		optional<Container> v;
			if( auto pSub=Global().TrySubContainer( container ); pSub )
				v = pSub->TrySubContainer( path );
		return v;
	}
}
#undef var
#undef $