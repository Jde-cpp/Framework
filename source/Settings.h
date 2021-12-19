#pragma once
#include "DateTime.h"
#include <jde/Assert.h>
#include <jde/Exports.h>
#include <jde/Exception.h>
#include <jde/Str.h>

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
	struct Γ Container
	{
		Container( const json& json )noexcept;
		Container( path jsonFile )noexcept(false);
		α TryMembers( sv path )noexcept->flat_map<string,Container>;
		α Have( sv path )noexcept->bool;
		α FindPath( sv path )const noexcept->optional<json>;
		ⓣ TryArray( sv path )noexcept->vector<T>;
		ⓣ Map( sv path )noexcept->flat_map<string,T>;

		α ForEach( sv path, function<void(sv, const nlohmann::json&)> f )noexcept->void;

		α SubContainer( sv entry )const noexcept(false)->sp<Container>;
		α TrySubContainer( sv entry )const noexcept->optional<Container>;
		ⓣ Get( sv path, SRCE )const noexcept(false)->T;
		ⓣ TryGet( sv path )const noexcept->optional<T>;

		α& Json()noexcept{ /*ASSERT(_pJson);*/ return *_pJson; }
	private:
		up<json> _pJson;
	};

	Γ α Global()noexcept->Container&;

	α FileStem()noexcept->string;
	#define $ template<> Ξ
	$ Container::Get<TimePoint>( sv path, const source_location& sl )const noexcept(false)->TimePoint{ return DateTime{ Get<string>(path, sl) }.GetTimePoint(); }
	$ Container::Get<fs::path>( sv path, const source_location& )const noexcept(false)->fs::path{ var p = TryGet<string>(path); return p.has_value() ? fs::path{*p} : fs::path{}; }

	ⓣ Container::Get( sv path, const source_location& sl )const noexcept(false)->T
	{
		auto p = TryGet<T>( path ); if( !p ) throw Exception{ sl, ELogLevel::Debug, "'{}' was not found in settings.", path };//mysql precludes using THROW_IF
		return *p;
	}

	$ Container::TryGet<Duration>( sv path )const noexcept->optional<Duration>
	{
		var strng = TryGet<string>( path );
		optional<std::chrono::system_clock::duration> result;
		if( strng.has_value() )
			Try( [strng, &result](){ result = Chrono::ToDuration(*strng);} );
		return  result;
	}
	$ Container::TryGet<ELogLevel>( sv path )const noexcept->optional<ELogLevel>
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

	$ Container::TryGet<fs::path>( sv path )const noexcept->optional<fs::path>
	{
		var p = TryGet<string>( path );
		return p ? optional<fs::path>(*p) : nullopt;
	}

	ⓣ Container::TryGet( sv path )const noexcept->optional<T>
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
		if( auto p = FindPath( path ); p )
		{
			for( auto& i : p->items() )
				values.emplace_back( i );
		}
		return values;
	}
	Γ α Set( sv path, const fs::path& value )noexcept->void;

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


	ⓣ Get( sv path, SRCE )noexcept(false){ return Global().Get<T>( path, sl ); }
	ⓣ TryGet( sv path )noexcept->optional<T>{ return Global().TryGet<T>( path ); }
	$ TryGet<Duration>( sv path )noexcept->optional<Duration>{ return Global().TryGet<Duration>( path ); }
	Ξ TryMembers( sv path )noexcept->flat_map<string,Container>{ return Global().TryMembers( path ); }
	$ TryGet<ELogLevel>( sv path )noexcept->optional<ELogLevel>{ return Global().TryGet<ELogLevel>( path ); }
	Ξ ForEach( sv path, function<void(sv, const nlohmann::json& v)> f )noexcept->void{ return Global().ForEach(path, f); }

	Τ struct Item
	{
		Item( sv path, T dflt ):
			Value{ TryGet<T>(path).value_or(dflt) }
		{}
		operator T(){return Value;}
		const T Value;
	};

	ⓣ TryGetSubcontainer( sv container, sv path )noexcept->optional<T>
	{
		optional<T> v;
		if( auto pSub=Global().TrySubContainer( container ); pSub )
			v = pSub->TryGet<T>( path );
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