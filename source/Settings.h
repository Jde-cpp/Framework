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
#define 🚪 JDE_NATIVE_VISIBILITY auto
namespace Jde::Settings
{
	α Path()noexcept->fs::path;
	using nlohmann::json;
	struct JDE_NATIVE_VISIBILITY Container
	{
		Container( const json& json )noexcept;
		Container( path jsonFile )noexcept(false);
		α TryMembers( sv path )noexcept->flat_map<string,Container>;
		bool Have( sv path )noexcept;
		α FindPath( sv path )const noexcept->optional<json>;
		ⓣ TryArray( sv path )noexcept->vector<T>;
		ⓣ Map( sv path )noexcept->map<string,T>;

		α ForEach( sv path, function<void(sv, const nlohmann::json&)> f )noexcept->void;

		sp<Container> SubContainer( sv entry )const noexcept(false);
		optional<Container> TrySubContainer( sv entry )const noexcept;
		ⓣ Get( sv path )const noexcept(false)->T;
		ⓣ TryGet( sv path )const noexcept->optional<T>;
		json& Json()noexcept{ /*ASSERT(_pJson);*/ return *_pJson; }
	private:
		unique_ptr<json> _pJson;
	};

	🚪 Global()noexcept->Container&;

	string FileStem()noexcept;
	#define $ template<> inline α
	$ Container::Get<TimePoint>( sv path )const noexcept(false)->TimePoint{ return DateTime{ Get<string>(path) }.GetTimePoint(); }
	$ Container::Get<fs::path>( sv path )const noexcept(false)->fs::path{ var p = TryGet<string>(path); return p.has_value() ? fs::path{*p} : fs::path{}; }

	ⓣ Container::Get( sv path )const noexcept(false)->T
	{
		auto p = TryGet<T>( path ); THROW_IFX( !p, EnvironmentException("'{}' was not found in settings.", path) );
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

	inline α Container::TryMembers( sv path )noexcept->flat_map<string,Container>
	{
		flat_map<string,Container> members;
		auto j = FindPath( path );
		if( j->is_object() )
		{
			auto obj = j->get<json::object_t>();
			for( auto& [name,j] : obj )
				members.emplace( name, Container{j} );
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

	ⓣ Container::Map( sv path )noexcept->map<string,T>
	{
		auto pItem = _pJson->find( path );
		map<string,T> values;
		if( pItem!=_pJson->end() )
		{
			for( var& [key,value] : pItem->items() )
				values.emplace( key, value );
		}
		return values;
	}

	inline α Container::ForEach( sv path, function<void(sv, const nlohmann::json&)> f )noexcept->void
	{

		if( auto p = FindPath( path ); p && p->is_object() )
		{
			for( auto&& i : p->items() )
				f( i.key(), i.value() );
		}
	}


	ⓣ Get( sv path )noexcept(false){ return Global().Get<T>( path ); }
	ⓣ TryGet( sv path )noexcept->optional<T>{ return Global().TryGet<T>( path ); }
	$ TryGet<Duration>( sv path )noexcept->optional<Duration>{ return Global().TryGet<Duration>( path ); }
	inline α TryMembers( sv path )noexcept->flat_map<string,Container>{ return Global().TryMembers( path ); }
	$ TryGet<ELogLevel>( sv path )noexcept->optional<ELogLevel>{ return Global().TryGet<ELogLevel>( path ); }
	inline α ForEach( sv path, function<void(sv, const nlohmann::json& v)> f )noexcept->void{ return Global().ForEach(path, f); }
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
#undef 🚪
#undef $