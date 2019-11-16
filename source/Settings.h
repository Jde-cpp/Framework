#pragma once
//#include <nlohmann/json.hpp>
#include "Exports.h"

#define var const auto
namespace Jde::Settings
{
	using nlohmann::json;
	struct JDE_NATIVE_VISIBILITY Container
	{
		Container( const nlohmann::json& json );
		Container( const fs::path& jsonFile );
		Jde::Duration Duration( string_view path, const Jde::Duration& dflt )noexcept;
		bool Bool( string_view path, bool dflt )noexcept;
		string String( string_view path )noexcept;
		uint16 Uint16( string_view path )noexcept;
		
		template<typename T>
		vector<T> Array( string_view path )noexcept;

		//fs::path Path( string_view path, const fs::path& dflt=fs::path() )const noexcept;
		shared_ptr<Container> SubContainer( string_view entry )const throw();
		template<typename T>
		T Get( string_view path )const noexcept(false);
		template<typename T>
		T Get( string_view path, const T& defaultValue )const noexcept;
		nlohmann::json& Json()noexcept{ ASSERT(_pJson); return *_pJson; }
	private:
		unique_ptr<nlohmann::json> _pJson;
	};

	template<>
	inline fs::path Container::Get<fs::path>( string_view path )const noexcept(false)
	{ 
		string value = (*_pJson)[string(path)];
		if( !value.size() )
			THROW( EnvironmentException(fmt::format("'{}' was not found in settings.", path)) );

		return fs::path(value);
	}

	template<typename T>
	T Container::Get( string_view path )const noexcept(false)
	{
		auto item = _pJson->find( path );
		if( item==_pJson->end() )
			THROW( EnvironmentException(fmt::format("'{}' was not found in settings.", path)) );
		return item->get<T>();
	}

	template<typename T>
	vector<T> Container::Array( string_view path )noexcept
	{
		auto item = _pJson->find( path );
		if( item==_pJson->end() )
			THROW( EnvironmentException(fmt::format("'{}' was not found in settings.", path)) );
		vector<T> values;
		for( auto& element : *item )
			values.push_back( element.get<T>() );
		return values;
	}


	template<>
	inline fs::path Container::Get( string_view path, const fs::path& defaultValue )const noexcept
	{
		auto item = _pJson->find( path );
		return item==_pJson->end() ? defaultValue : fs::path( item->get<string>() );
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