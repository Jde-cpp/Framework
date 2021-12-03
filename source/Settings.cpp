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
			CHECK_PATH( jsonFile );
			var fileString = jsonFile.string();
			std::ifstream is( fileString.c_str() ); THROW_IF( is.bad(), "Could not open file:  {}", jsonFile.string() );
			is >> *_pJson;
		}
		Container::Container( const nlohmann::json& json )noexcept:_pJson{ make_unique<nlohmann::json>(json) }{}

		α Container::TrySubContainer( sv entry )const noexcept->optional<Container>
		{
			auto p = _pJson->find( entry );
			return p==_pJson->end() ? optional<Container>{} : Container( *p );
		}
		α Container::SubContainer( sv entry )const noexcept(false)->sp<Container>
		{
			auto item = _pJson->find( entry );
			THROW_IF( item==_pJson->end(), "Could not find {}", entry );
			return make_shared<Container>( *item );
		}

		α Container::Have( sv path )noexcept->bool{ return _pJson->find(path)!=_pJson->end(); }//TODO rename contains

		α Container::FindPath( sv path )const noexcept->optional<json>
		{
			var values = Str::Split( path, '/' );
			auto j = _pJson.get();
			optional<json> result;
			for( uint i=0; i<values.size(); ++i )
			{
				var value = values[i];
				auto p = j->find( value );
				if( p == j->end() )
					break;
				if( i+1==values.size() )
					result = *p;
				else
					j = &*p;
			}
			return result;
		}
	}

	α Settings::Set( sv path, const fs::path& value )noexcept->void
	{
		var values = Str::Split( path, '/' );
		auto j = &Global().Json();
		for( uint i=0; i<values.size()-1; ++i )
		{
			var member = values[i];
			auto p = j->find( member );
			if( p == j->end() )
			{
				(*j)[string{member}] = json::object();
				p = j->find( member );
			}
			j = &*p;
		}
		(*j)[string{values.back()}] = value.string();
	}

	α Settings::FileStem()noexcept->string
	{
		var executable = OSApp::Executable().filename();
#ifdef _MSC_VER
		return executable.string().starts_with( "Jde." ) ? OSApp::Executable().stem().extension().string().substr(1) : OSApp::Executable().stem().string();
#else
		return executable.string().starts_with( "Tests." ) ? executable.string() : OSApp::Executable().extension().string().substr( 1 );
#endif
	}
	α Settings::Path()noexcept->fs::path
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
	α Settings::Global()noexcept->Settings::Container&
	{
		if( !_pGlobal )
		{
			var settingsPath = Path();
			try
			{
				if( !fs::exists(settingsPath) )
					throw std::exception{ fmt::format("'{}' does not exist", settingsPath.string()).c_str() };//pre-main, no logging
				_pGlobal = mu<Jde::Settings::Container>( settingsPath );
				LOG_MEMORY( "settings", ELogLevel::Information, "({}) Settings", settingsPath.string() );
			}
			catch( const std::exception& e )
			{
				LOG_MEMORY( "settings", ELogLevel::Critical, "({})Could not load settings - {}", settingsPath.string(), e.what() );
				_pGlobal = std::make_unique<Jde::Settings::Container>( nlohmann::json{} );
			}
		}
		return *_pGlobal;
	}
}