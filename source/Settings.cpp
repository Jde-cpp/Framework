#include "Settings.h"
#include <jde/App.h>
//#include <fstream>
#include <jde/io/File.h>

#define var const auto

namespace Jde::Settings
{
	static const LogTag& _logLevel = Logging::TagLevel( "settings" );
	up<Settings::Container> _pGlobal;

	Container::Container( path jsonFile, SL sl )ε:
		_pJson{ mu<nlohmann::json>() }
	{
		CHECK_PATH( jsonFile, sl );
		var fileString = jsonFile.string();
		std::ifstream is( fileString.c_str() ); THROW_IF( is.bad(), "Could not open file:  {}", jsonFile.string() );
		is >> *_pJson;
	}
	Container::Container( const nlohmann::json& json )noexcept:_pJson{ make_unique<nlohmann::json>(json) }{}

	α Container::TrySubContainer( sv entry )Ι->optional<Container>
	{
		auto p = _pJson->find( entry );
		return p==_pJson->end() ? optional<Container>{} : Container( *p );
	}
	α Container::SubContainer( sv entry )Ε->sp<Container>
	{
		auto item = _pJson->find( entry );
		THROW_IF( item==_pJson->end(), "Could not find {}", entry );
		return make_shared<Container>( *item );
	}

	α Container::Have( sv path )noexcept->bool{ return _pJson->find(path)!=_pJson->end(); }//TODO rename contains

	α Container::FindPath( sv path )Ι->optional<json>
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

	JsonNumber::JsonNumber( json j )ε
	{
		if( j.is_number_float() )
			Value = j.get<double>();
		else if( j.is_number_unsigned() )
			Value = j.get<uint>();
		else
			THROW( "{} not implemented", (uint)j.type() );
	}
	Container::Variant Container::operator[]( sv path )ε
	{
		auto j = FindPath( path );
		Variant result;
		if( j )
		{
			if( j->is_string() )
				result = j->get<string>();
			else
				result = JsonNumber{ *j };
		}
		return result;
	}
}
namespace Jde
{
	α Settings::Save( const json& j, sv what, SL sl )ε->void
	{
		var path = Path();
		IO::FileUtilities::Save( path, j.dump(), std::ios_base::out, sl );
		LOG( "({})Saved for '{}'", path, what );
	}
	α Settings::Set( sv what, const Container::Variant& v, bool save, SL sl )ε->void
	{
		var values = Str::Split( what, '/' );
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
		if( v.index()==0 )
			(*j)[string{values.back()}] = nullptr;
		else if( v.index()==1 )
			(*j)[string{values.back()}] = get<string>( v );
		else if( v.index()==2 )
		{
			auto& number = get<JsonNumber>( v ).Value;
			if( number.index()==0 )
				(*j)[string{values.back()}] = get<double>( number );
			else if( number.index()==1 )
				(*j)[string{values.back()}] = get<_int>( number );
			else if( number.index()==2 )
				(*j)[string{values.back()}] = get<uint>( number );
		}
		if( save )
			Save( Global().Json(), what, sl );
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
				LOG_MEMORY( {}, ELogLevel::Information, "current_path={}", std::filesystem::current_path() );
				if( !fs::exists(settingsPath) )
					throw std::runtime_error{ "file does not exist" };
				_pGlobal = mu<Jde::Settings::Container>( settingsPath );
				LOG_MEMORY( {}, ELogLevel::Information, "Settings path={}", settingsPath.string() );

			}
			catch( const std::exception& e )
			{
				BREAK;
				LOG_MEMORY( {}, ELogLevel::Critical, "({})Could not load settings - {}", settingsPath.string(), e.what() );
				_pGlobal = std::make_unique<Jde::Settings::Container>( nlohmann::json{} );
			}
		}
		return *_pGlobal;
	}
}