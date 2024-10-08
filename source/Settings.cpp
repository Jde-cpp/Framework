﻿#include "Settings.h"
#include <jde/App.h>
//#include <fstream>
#include <jde/io/File.h>
#include <regex>

#define var const auto

namespace Jde::Settings
{
	static sp<Jde::LogTag> _logTag = Logging::Tag( "settings" );
	α LogTag()ι->sp<Jde::LogTag>&{ return _logTag; }
	up<Settings::Container> _pGlobal;

	Container::Container( const fs::path& jsonFile, SL sl )ε:
		_pJson{ mu<nlohmann::json>() }{
		CHECK_PATH( jsonFile, sl );
		var fileString = jsonFile.string();
		std::ifstream is( fileString.c_str() ); THROW_IF( is.bad(), "Could not open file:  {}", jsonFile.string() );
		is >> *_pJson;
	}

	Container::Container( const nlohmann::json& json )ι:_pJson{ mu<nlohmann::json>(json) }{}

	α Container::TrySubContainer( sv entry )Ι->optional<Container>{
		auto p = _pJson->find( entry );
		return p==_pJson->end() ? optional<Container>{} : Container( *p );
	}

	α Container::SubContainer( sv entry )Ε->sp<Container>{
		auto item = _pJson->find( entry );
		THROW_IF( item==_pJson->end(), "Could not find {}", entry );
		return make_shared<Container>( *item );
	}

	α Container::Have( sv path )ι->bool{ return _pJson->find(path)!=_pJson->end(); }//TODO rename contains

	α Container::FindPath( sv path )Ι->optional<json>{
		var values = Str::Split( path, '/' );
		auto j = _pJson.get();
		optional<json> result;
		for( uint i=0; i<values.size(); ++i ){
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

	JsonNumber::JsonNumber( json j )ε{
		if( j.is_number_float() )
			Value = j.get<double>();
		else if( j.is_number_unsigned() )
			Value = j.get<uint>();
		else
			THROW( "{} not implemented", (uint)j.type() );
	}
	Container::Variant Container::operator[]( sv path )ε{
		auto j = FindPath( path );
		Variant result;
		if( j ){
			if( j->is_string() )
				result = j->get<string>();
			else
				result = JsonNumber{ *j };
		}
		return result;
	}
}
namespace Jde{
	α Settings::Save( const json& j, sv what, SL sl )ε->void{
		var path = Path();
		IO::FileUtilities::Save( path, j.dump(), std::ios_base::out, sl );
		DBG( "({})Saved for '{}'", path.string(), what );
	}

	α Settings::Set( sv what, const Container::Variant& v, bool save, SL sl )ε->void{
		var paths = Str::Split( what, '/' );
		auto parent = &Global().Json();
		for( uint i=0; i<paths.size()-1; ++i ){
			var subPath = paths[i];
			auto p = parent->find( subPath );
			if( p == parent->end() ){
				(*parent)[string{subPath}] = json::object();
				p = parent->find( subPath );
			}
			parent = &*p;
		}
		string text = "null";
		const string member{ paths.back() };
		auto pOriginal = parent->find( member );
		var original = pOriginal==parent->end() ? "null" : pOriginal->dump();
		if( v.index()==0 )
			(*parent)[member] = nullptr;
		else if( v.index()==1 )
			(*parent)[member] = text = get<string>( v );
		else if( v.index()==2 ){
			auto& number = get<JsonNumber>( v ).Value;
			text = std::visit( [](auto& x){return std::to_string(x);}, number );
			if( number.index()==0 )
				(*parent)[member] = get<double>( number );
			else if( number.index()==1 )
				(*parent)[member] = get<_int>( number );
			else if( number.index()==2 )
				(*parent)[member] = get<uint>( number );
		}
		if( save )
			Save( Global().Json(), what, sl );
		else
			LOGSL( ELogLevel::Information, _logTag, "Changing '{}':  '{}' => '{}'", what, original, text );
	}

	α Settings::FileStem()ι->string{
		var executable = OSApp::Executable().filename();
#ifdef _MSC_VER
		auto stem = executable.stem().string();
		return stem.size()>4 ? stem.substr(4) : stem;
#else
		return executable.string().starts_with( "Jde." ) ? fs::path( executable.string().substr(4) ) : executable;
#endif
	}
	α Settings::Path()ι->fs::path{
		static fs::path _path;
		if( _path.empty() ){
			var fileName = fs::path{ Jde::format("{}.json", FileStem()) };
			vector<fs::path> paths{ fileName, fs::path{"../config"}/fileName, fs::path{"config"}/fileName };
			auto p = find_if( paths, []( var& path ){return fs::exists(path);} );
			_path = p!=paths.end() ? *p : OSApp::ApplicationDataFolder()/fileName;
		}
		return _path;
	}
	α Settings::Global()ι->Settings::Container&{
		if( !_pGlobal ){
			var settingsPath = Path();
			try{
				LOG_MEMORY( {}, ELogLevel::Information, "current_path={}", std::filesystem::current_path() );
				if( !fs::exists(settingsPath) )
					throw std::runtime_error{ "file does not exist" };
				_pGlobal = mu<Jde::Settings::Container>( settingsPath );
				LOG_MEMORY( {}, ELogLevel::Information, "Settings path={}", settingsPath.string() );
			}
			catch( const std::exception& e ){
				BREAK;
				LOG_MEMORY( {}, ELogLevel::Critical, "({})Could not load settings - {}", settingsPath.string(), e.what() );
				_pGlobal = mu<Jde::Settings::Container>( nlohmann::json{} );
			}
		}
		return *_pGlobal;
	}

	α Settings::Env( sv path, SL sl )ι->optional<string>{
		auto setting = Global().Get( path );
		if( setting ){
			std::regex regex( "\\$\\((.+?)\\)" );
			for(;;){
				auto begin = std::sregex_iterator( setting->begin(), setting->end(), regex );
				if( begin==std::sregex_iterator() )
					break;
				std::smatch b = *begin;
				string match = begin->str();
				string group = match.substr( 2, match.size()-3 );
				var env = OSApp::EnvironmentVariable( group, sl ).value_or( "" );
				setting = Str::Replace( *setting, match, env );
			}
		}
		return setting;
	}

	α Settings::Envɛ( sv path, SL sl )ε->string{
		auto y = Env( path, sl );
		THROW_IF( !y, "${} not found in settings", path );
		return *y;
	}
}