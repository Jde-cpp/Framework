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
				THROWX( EnvironmentException("file does not exsist:  {}", jsonFile.string()) );
			var fileString = jsonFile.string();
			std::ifstream is( fileString.c_str() );
			if( is.bad() )
				THROWX( EnvironmentException("Could not open file:  {}", jsonFile.string()) );
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
			THROW_IFX( item==_pJson->end(), EnvironmentException("Could not find {}", entry) );
			return make_shared<Container>( *item );
		}

		bool Container::Have( sv path )noexcept{ return _pJson->find(path)!=_pJson->end(); }//TODO rename contains

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
					break;
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

}