#pragma once
#include "DateTime.h"
#include "JdeAssert.h"
#include "Exports.h"

#define var const auto
namespace Jde::Settings
{
	using nlohmann::json;
	struct JDE_NATIVE_VISIBILITY Container
	{
		Container( const nlohmann::json& json )noexcept;
		Container( path jsonFile )noexcept(false);
		//Jde::Duration Duration( string_view path, const Jde::Duration& dflt )noexcept;
		bool Bool( string_view path, bool dflt )noexcept;
		bool Have( string_view path )noexcept;
		string String( string_view path )noexcept;
		uint16 Uint16( string_view path )noexcept;

		template<typename T>
		vector<T> Array( string_view path )noexcept(false);

		template<typename TValue>
		map<string,TValue> Map( string_view path )noexcept;

		//fs::path Path( string_view path, path dflt=fs::path() )const noexcept;
		shared_ptr<Container> SubContainer( string_view entry )const noexcept(false);
		template<typename T>
		T Get( string_view path )const noexcept(false);
		template<typename T>
		T Get( string_view path, T defaultValue )const noexcept;

//		void Set( const string& path, uint size )noexcept;
		template<typename T>
		optional<T> Get2( string_view path )const noexcept;

		nlohmann::json& Json()noexcept{ /*ASSERT(_pJson);*/ return *_pJson; }
	private:
		unique_ptr<nlohmann::json> _pJson;
	};

	template<> inline TimePoint Container::Get<TimePoint>( string_view path )const noexcept(false){ return DateTime{ Get<string>(path) }.GetTimePoint(); }
	template<> inline fs::path Container::Get<fs::path>( string_view path )const noexcept(false){ return fs::path{ Get<string>(path) }; }

	template<typename T>
	T Container::Get( string_view path )const noexcept(false)
	{
		auto item = _pJson->find( path );
		if( item==_pJson->end() )
			THROW( EnvironmentException(fmt::format("'{}' was not found in settings.", path)) );
		return item->get<T>();
	}

	template<>
	inline optional<Duration> Container::Get2<Duration>( string_view path )const noexcept
	{
		var strng = Get2<string>( path );
		optional<std::chrono::system_clock::duration> result;
		if( strng.has_value() )
			Try( [strng, &result](){ result = Chrono::ToDuration(*strng);} );
		return  result;
	}

	template<typename T>
	optional<T> Container::Get2( string_view path )const noexcept
	{
		auto item = _pJson->find( path );
		return item==_pJson->end() ? optional<T>{} : optional<T>{ item->get<T>() };
	}

	template<typename T>
	vector<T> Container::Array( string_view path )noexcept(false)
	{
		auto item = _pJson->find( path );
		if( item==_pJson->end() )
			THROW( EnvironmentException(fmt::format("'{}' was not found in settings.", path)) );
		vector<T> values;
		for( auto& element : *item )
			values.push_back( element.get<T>() );
		return values;
	}

	template<typename TValue>
	map<string,TValue> Container::Map( string_view path )noexcept
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
	inline fs::path Container::Get( string_view path, fs::path defaultValue )const noexcept
	{
		auto item = _pJson->find( path );
		return item==_pJson->end() ? defaultValue : fs::path( item->get<string>() );
	}

	template<typename T>
	T Container::Get( string_view path, T defaultValue )const noexcept
	{
		auto item = _pJson->find( path );
		return item==_pJson->end() ? defaultValue : item->get<T>();
	}


	JDE_NATIVE_VISIBILITY Container& Global();
	JDE_NATIVE_VISIBILITY void SetGlobal( shared_ptr<Container> container );

	struct Server
	{
		string Name;
		uint16 Port;
	};

	JDE_NATIVE_VISIBILITY void to_json( nlohmann::json& j, const Server& server );
	JDE_NATIVE_VISIBILITY void from_json( const nlohmann::json& j, Server& server );
}
#undef var