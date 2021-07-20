#pragma once
#include "DateTime.h"
#include <jde/Assert.h>
#include <jde/Exports.h>
#include <jde/Exception.h>

#pragma warning(push)
#pragma warning( disable : 4715)
#include <nlohmann/json.hpp>
#pragma warning(pop)

#define var const auto
#define 🚪 JDE_NATIVE_VISIBILITY auto
namespace Jde::Settings
{
	using nlohmann::json;
	struct JDE_NATIVE_VISIBILITY Container
	{
		Container( const nlohmann::json& json )noexcept;
		Container( path jsonFile )noexcept(false);
		//Jde::Duration Duration( sv path, const Jde::Duration& dflt )noexcept;
		bool Bool( sv path, bool dflt )noexcept;
		bool Have( sv path )noexcept;
		string String( sv path )noexcept;
		uint16 Uint16( sv path )noexcept;

		template<typename T>
		vector<T> Array( sv path )noexcept(false);

		template<typename TValue>
		map<string,TValue> Map( sv path )noexcept;

		//fs::path Path( sv path, path dflt=fs::path() )const noexcept;
		sp<Container> SubContainer( sv entry )const noexcept(false);
		optional<Container> TrySubContainer( sv entry )const noexcept;
		template<typename T>
		T Get( sv path )const noexcept(false);
		template<typename T>
		T Get( sv path, T defaultValue )const noexcept;

		template<typename T>
		optional<T> Get2( sv path )const noexcept;

		nlohmann::json& Json()noexcept{ /*ASSERT(_pJson);*/ return *_pJson; }
	private:
		unique_ptr<nlohmann::json> _pJson;
	};

	template<> inline TimePoint Container::Get<TimePoint>( sv path )const noexcept(false){ return DateTime{ Get<string>(path) }.GetTimePoint(); }
	template<> inline fs::path Container::Get<fs::path>( sv path )const noexcept(false){ var p = Get2<string>(path); return p.has_value() ? fs::path{*p} : fs::path{}; }

	template<typename T>
	T Container::Get( sv path )const noexcept(false)
	{
		auto item = _pJson->find( path ); THROW_IFX( item == _pJson->end(), EnvironmentException("'{}' was not found in settings.", path) );
		return item->get<T>();
	}

	template<> inline α
	Container::Get2<Duration>( sv path )const noexcept->optional<Duration>
	{
		var strng = Get2<string>( path );
		optional<std::chrono::system_clock::duration> result;
		if( strng.has_value() )
			Try( [strng, &result](){ result = Chrono::ToDuration(*strng);} );
		return  result;
	}

	template<> inline α 
	Container::Get2<fs::path>( sv path )const noexcept->optional<fs::path>
	{
		var p = Get2<string>( path );
		return p ? optional<fs::path>(*p) : std::nullopt;
	}

	template<typename T>
	optional<T> Container::Get2( sv path )const noexcept
	{
		auto item = _pJson->find( path );
		return item==_pJson->end() ? optional<T>{} : optional<T>{ item->get<T>() };
	}

	template<typename T>
	vector<T> Container::Array( sv path )noexcept(false)
	{
		auto item = _pJson->find( path ); THROW_IFX( item == _pJson->end(), EnvironmentException("'{}' was not found in settings.", path) );
		vector<T> values;
		for( auto& element : *item )
			values.push_back( element.get<T>() );
		return values;
	}

	template<typename TValue>
	map<string,TValue> Container::Map( sv path )noexcept
	{
		auto pItem = _pJson->find( path );
		map<string,TValue> values;
		if( pItem!=_pJson->end() )
		{
			for( var& [key,value] : pItem->items() )
				values.emplace( key, value );
		}
		return values;
	}

	template<>
	inline fs::path Container::Get( sv path, fs::path defaultValue )const noexcept
	{
		auto item = _pJson->find( path );
		return item==_pJson->end() ? defaultValue : fs::path( item->get<string>() );
	}

	template<typename T>
	T Container::Get( sv path, T defaultValue )const noexcept
	{
		auto item = _pJson->find( path );
		return item==_pJson->end() ? defaultValue : item->get<T>();
	}


	JDE_NATIVE_VISIBILITY Container& Global()noexcept;
	JDE_NATIVE_VISIBILITY sp<Container> GlobalPtr()noexcept;
	JDE_NATIVE_VISIBILITY void SetGlobal( sp<Container> container )noexcept;

	ⓣ Get( sv path )noexcept{ return Global().Get2<T>( path ); }

	ⓣ TryGetSubcontainer( sv container, sv path )noexcept->optional<T>
	{
		optional<T> v;
		if( auto p=Settings::GlobalPtr(); p )
		{
			if( auto pSub=p->TrySubContainer( container ); pSub )
				v = pSub->Get2<T>( path );
		}
		return v;
	}
	
	template<> inline auto TryGetSubcontainer<Container>( sv container, sv path )noexcept->optional<Container>
	{
		optional<Container> v;
		if( auto p=Settings::GlobalPtr(); p )
		{
			if( auto pSub=p->TrySubContainer( container ); pSub )
				v = pSub->TrySubContainer( path );
		}
		return v;
	}

	struct Server
	{
		string Name;
		uint16 Port;
	};

	JDE_NATIVE_VISIBILITY void to_json( nlohmann::json& j, const Server& server );
	JDE_NATIVE_VISIBILITY void from_json( const nlohmann::json& j, Server& server );
}
#undef var
#undef 🚪