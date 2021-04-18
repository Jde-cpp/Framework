#include "Settings.h"
#include <fstream>

#define var const auto

namespace Jde::Settings
{
	sp<Container> _pGlobal;
	Container& Global()noexcept{ ASSERT(_pGlobal); return *_pGlobal;}
	sp<Container> GlobalPtr()noexcept{return _pGlobal;}
	void SetGlobal( sp<Container> settings )noexcept{ _pGlobal = settings; }

	Container::Container( path jsonFile )noexcept(false):
		_pJson{ make_unique<nlohmann::json>() }
	{
		if( !fs::exists(jsonFile) )
			THROW( EnvironmentException("file does not exsist:  {}", jsonFile.string()) );
		var fileString = jsonFile.string();
		std::ifstream is( fileString.c_str() );
		if( is.bad() )
			THROW( EnvironmentException("Could not open file:  {}", jsonFile.string()) );
		is >> *_pJson;
	}
	Container::Container( const nlohmann::json& json )noexcept:
		_pJson{ make_unique<nlohmann::json>(json) }
	{}

	optional<Container> Container::TrySubContainer( sv entry )const noexcept
	{
		auto p = _pJson->find( entry );
		return p==_pJson->end() ? optional<Container>{} : Container( *p );
	}
	sp<Container> Container::SubContainer( sv entry )const noexcept(false)
	{
		auto item = _pJson->find( entry );
		if( item==_pJson->end() )
			THROW( EnvironmentException(fmt::format("Could not find {}", entry)) );
		return make_shared<Container>( *item );
	}

/*	void Container::Set( str path, uint size )noexcept
	{
		//nlohmann::json j;
		//j[string{path}] = 23;
		(*_pJson)[path] = size;
	}

	Jde::Duration Container::Duration( sv / *path* /, const Jde::Duration& dflt )noexcept
	{
		//var string = String( path );
		//return string.size() ? TimeSpan::Parse(string) : ;
		//var x = Get<TimePoint>("foo");
		return dflt;//TODO implement
	}*/

	// fs::path Container::Path( sv path, path dflt )const noexcept
	// {
	// 	auto pEntry = _pJson->find( path );
	// 	return pEntry==_pJson->end() ? dflt : fs::path( pEntry->get<string>() );
	// }

	bool Container::Bool( sv path, bool dflt )noexcept
	{
		return _pJson->find(path)==_pJson->end() ? dflt : (*_pJson)[string(path)].get<bool>();
	}
	bool Container::Have( sv path )noexcept//TODO rename contains
	{
		return _pJson->find(path)!=_pJson->end();
	}

	string Container::String( sv path )noexcept
	{
		return _pJson->find(path)==_pJson->end() ? string() : (*_pJson)[string(path)].get<string>();
	}

	uint16 Container::Uint16( sv path )noexcept
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