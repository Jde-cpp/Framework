#include "stdafx.h"
#include "Settings.h"
#include <fstream>
//#include "../Exception.h"

#define var const auto

namespace Jde::Settings
{
	shared_ptr<Container> _pGlobal;
	Container& Global(){ ASSERT(_pGlobal); return *_pGlobal;}
	void SetGlobal( shared_ptr<Container> settings ){ _pGlobal = settings; }

	Container::Container( const fs::path& jsonFile ):
		_pJson{ make_unique<nlohmann::json>() }
	{
		if( !fs::exists(jsonFile) )
			THROW( IOException("file does not exsist:  {}", jsonFile.string()) );
		var fileString = jsonFile.string();
		std::ifstream is( fileString.c_str() );
		if( is.bad() )
			THROW( IOException("Could not open file:  {}", jsonFile.string()) );
		is >> *_pJson;
	}
	Container::Container( const nlohmann::json& json ):
		_pJson{ make_unique<nlohmann::json>(json) }
	{}


	template<>
	bool Container::Get( string_view path, const bool& defaultValue )const noexcept
	{
		auto item = _pJson->find( path );
		return item==_pJson->end() ? defaultValue : item->get<bool>();
	}

	template<>
	int Container::Get( string_view path, const int& defaultValue )const noexcept
	{
		auto item = _pJson->find( path );
		return item==_pJson->end() ? defaultValue : item->get<int>();
	}

	shared_ptr<Container> Container::SubContainer( string_view entry )const throw()
	{
		auto item = _pJson->find( entry );
		if( item==_pJson->end() )
			THROW( EnvironmentException(fmt::format("Could not find {}", entry)) );
		return make_shared<Container>( *item );
	}

	
	Jde::Duration Container::Duration( string_view /*path*/, const Jde::Duration& dflt )noexcept
	{
		//var string = String( path );
		//return string.size() ? TimeSpan::Parse(string) : ;
		//var x = Get<TimePoint>("foo");
		return dflt;//TODO implement
	}

	// fs::path Container::Path( string_view path, const fs::path& dflt )const noexcept
	// {
	// 	auto pEntry = _pJson->find( path );
	// 	return pEntry==_pJson->end() ? dflt : fs::path( pEntry->get<string>() );
	// }

	bool Container::Bool( string_view path, bool dflt )noexcept
	{
		return _pJson->find(path)==_pJson->end() ? dflt : (*_pJson)[string(path)].get<bool>();
	}
	
	string Container::String( string_view path )noexcept
	{
		return _pJson->find(path)==_pJson->end() ? string() : (*_pJson)[string(path)].get<string>();
	}

	uint16 Container::Uint16( string_view path )noexcept
	{
		return _pJson->find(path)==_pJson->end() ? 0 : (*_pJson)[string(path)].get<uint16>();
	}

	void to_json( nlohmann::json& j, const Server& server ) 
	{
		j = nlohmann::json{ {"name", server.Name}, {"port", server.Port} };
	}

	void from_json( const nlohmann::json& j, Server& server )
	{
		if( j.find("name")!=j.end() )
			j.at("name").get_to( server.Name );
		if( j.find("port")!=j.end() )
			j.at("port").get_to( server.Port );
	}
}