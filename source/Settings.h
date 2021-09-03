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
		//bool Bool( sv path, bool dflt )noexcept;
		bool Have( sv path )noexcept;
		//string String( sv path )noexcept;
		//uint16 Uint16( sv path )noexcept;

		α FindPath( sv path )const noexcept->optional<json>;
		ⓣ TryArray( sv path )noexcept->vector<T>;
		ⓣ Map( sv path )noexcept->map<string,T>;

		//fs::path Path( sv path, path dflt=fs::path() )const noexcept;
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
		var pText = TryGet<string>( path );
		return pText && pText->size() ? Str::ToEnum<ELogLevel,array<sv,7>>(ELogLevelStrings, *pText) : nullopt;
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
		return p ? optional<T>{ p->get<T>() } : nullopt;
		/*var values = Str::Split( path, "/" );
		Container container{ *_pJson };
		optional<T> result;
		for( uint i=0; i<values.size(); ++i )
		{
			var value = values[i];
			auto p = container.Json().find( value );
			if( p == container.Json().end() )
				break;
			if( i+1==values.size() )
				result = p->get<T>();
			else
				container = Container{*p};
		}
		return result;*/
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
	ⓣ TryArray( sv path )noexcept{ return Global().TryArray<T>(path); }//vector<Container>

	ⓣ Container::TryArray( sv path )noexcept->vector<T>
	{
		vector<T> values;
		if( auto p = _pJson->find(path); p!=_pJson->end() )
		{
			for( auto& element : *p )
				values.push_back( element.get<T>() );
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

/*	template<>
	inline fs::path Container::Get( sv path, fs::path defaultValue )const noexcept
	{
		auto item = _pJson->find( path );
		return item==_pJson->end() ? defaultValue : fs::path( item->get<string>() );
	}

	ⓣ
	T Container::Get( sv path, T defaultValue )const noexcept
	{
		auto item = _pJson->find( path );
		return item==_pJson->end() ? defaultValue : item->get<T>();
	}
*/

	ⓣ Get( sv path )noexcept(false){ return Global().Get<T>( path ); }
	ⓣ TryGet( sv path )noexcept->optional<T>{ return Global().TryGet<T>( path ); }
	$ TryGet<Duration>( sv path )noexcept->optional<Duration>{ return Global().TryGet<Duration>( path ); }
	inline α TryMembers( sv path )noexcept->flat_map<string,Container>{ return Global().TryMembers( path ); }
	$ TryGet<ELogLevel>( sv path )noexcept->optional<ELogLevel>{ return Global().TryGet<ELogLevel>( path ); }

	//JDE_NATIVE_VISIBILITY sp<Container> GlobalPtr()noexcept;
	//JDE_NATIVE_VISIBILITY void SetGlobal( sp<Container> container )noexcept;
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
/*
	struct Server
	{
		string Name;
		uint16 Port;
	};

	JDE_NATIVE_VISIBILITY void to_json( json& j, const Server& server );
	JDE_NATIVE_VISIBILITY void from_json( const json& j, Server& server );
*/
}
#undef var
#undef 🚪
#undef $