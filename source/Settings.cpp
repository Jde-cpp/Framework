#include "Settings.h"
#include <jde/App.h>
#include <fstream>

#define var const auto

namespace Jde
{
	namespace Settings
	{
		up<Settings::Container> _pGlobal;

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
		Container::Container( const nlohmann::json& json )noexcept:_pJson{ make_unique<nlohmann::json>(json) }{}

		optional<Container> Container::TrySubContainer( sv entry )const noexcept
		{
			auto p = _pJson->find( entry );
			return p==_pJson->end() ? optional<Container>{} : Container( *p );
		}
		sp<Container> Container::SubContainer( sv entry )const noexcept(false)
		{
			auto item = _pJson->find( entry );
			THROW_IFX( item==_pJson->end(), EnvironmentException(fmt::format("Could not find {}", entry)) );
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

		//bool Container::Bool( sv path, bool dflt )noexcept{ return _pJson->find(path)==_pJson->end() ? dflt : (*_pJson)[string(path)].get<bool>(); }
		bool Container::Have( sv path )noexcept{ return _pJson->find(path)!=_pJson->end(); }//TODO rename contains
		//string Container::String( sv path )noexcept{ return _pJson->find(path)==_pJson->end() ? string() : (*_pJson)[string(path)].get<string>(); }
		//uint16 Container::Uint16( sv path )noexcept{ return _pJson->find(path)==_pJson->end() ? 0 : (*_pJson)[string(path)].get<uint16>(); }

		α Container::FindPath( sv path )const noexcept->optional<json>
		{
			var values = Str::Split( path, '/' );
			auto j = *_pJson;
			optional<json> result;
			for( uint i=0; i<values.size(); ++i )
			{
				var value = values[i];
				auto p = j.find( value );
				if( p == j.end() )
				{
					//std::cout << "[" << path << "]" << j.dump() << endl;
					break;
				}
				if( i+1==values.size() )
					result = *p;
				else
					j = *p;
			}
			return result;
		}
	}
	string Settings::FileStem()noexcept
	{
#ifdef _MSC_VER
		return OSApp::Executable().stem().string();
#else
		var executable = OSApp::Executable().filename();
		return executable.filename().string().starts_with( "Tests." ) ? executable.filename().string() : OSApp::Executable().extension().string().substr( 1 );
#endif
	}
	fs::path Settings::Path()noexcept
	{
		var fileName = fs::path{ format("{}.json", FileStem()) };
		fs::path settingsPath{ fileName };
		if( !fs::exists(settingsPath) )
		{
			var settingsPathB = fs::path{".."}/fileName;
			settingsPath = fs::exists( settingsPathB ) ? settingsPathB : OSApp::ApplicationDataFolder()/fileName;
		}
		return settingsPath;
	}
	Settings::Container& Settings::Global()noexcept
	{
		if( !_pGlobal )
		{
			var settingsPath = Path();
			_pGlobal = fs::exists(settingsPath) ? std::make_unique<Jde::Settings::Container>( settingsPath ) : std::make_unique<Jde::Settings::Container>( nlohmann::json{} );
		}
		return *_pGlobal;
	}

/*	void to_json( nlohmann::json& j, const Server& server )
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
	*/
}