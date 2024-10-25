#include <jde/framework/settings.h>
#include <regex>
#include <jde/framework/process.h>
#include <jde/framework/str.h>
#include <jde/framework/io/file.h>
#include "DateTime.h"

#define let const auto


namespace Jde{
	constexpr ELogTags _tags{ ELogTags::Settings };
	up<jvalue> _settings; //up so doesn't get initialized 2x.
	α Settings::Value()ι->const jvalue&{ 
		if( !_settings )
			Load();
		return *_settings; 
	}

	α Settings::FindDuration( sv path )ι->optional<Duration>{
		optional<Duration> y;
		if( let setting = FindSV(path); setting.has_value() )
			Try( [setting, &y](){ y = Chrono::ToDuration(*setting);} );
		return y;
	}

	α Settings::FindString( sv path )ι->optional<string>{
		auto setting = Json::FindString( Value(), path );
		if( setting ){
			std::regex regex( "\\$\\((.+?)\\)" );
			for(;;){
				auto begin = std::sregex_iterator( setting->begin(), setting->end(), regex );
				if( begin==std::sregex_iterator() )
					break;
				std::smatch b = *begin;
				let match = begin->str();
				let group = match.substr( 2, match.size()-3 );
				let env = OSApp::EnvironmentVariable( group ).value_or( "" );
				setting = Str::Replace( *setting, match, env );
			}
		}
		return setting;
	}

	α Settings::FileStem()ι->string{
		let executable = OSApp::Executable().filename();
	#ifdef _MSC_VER
			auto stem = executable.stem().string();
			return stem.size()>4 ? stem.substr(4) : stem;
	#else
			return executable.string().starts_with( "Jde." ) ? fs::path( executable.string().substr(4) ) : executable;
	#endif
		}

	α Path()ι->fs::path{
		static fs::path _path;
		let fileName = fs::path{ Ƒ("{}.jsonnet", Settings::FileStem()) };
		vector<fs::path> paths{ fileName, fs::path{"../config"}/fileName, fs::path{"config"}/fileName };
		auto p = find_if( paths, []( let& path ){return fs::exists(path);} );
		_path = p!=paths.end() ? *p : OSApp::ApplicationDataFolder()/fileName;
		Information{ _tags, "settings path={}", _path.string() };
		return _path;
	}
	α SetEnv( jobject& j )->void{
		for( auto& [key,value] : j ){
			if( value.is_string() ){
				string setting{ value.get_string() };
				std::regex regex( "\\$\\((.+?)\\)" );
				for(;;){
					auto begin = std::sregex_iterator( setting.begin(), setting.end(), regex );
					if( begin==std::sregex_iterator() )
						break;
					std::smatch b = *begin;
					let match = begin->str();
					let group = match.substr( 2, match.size()-3 );
					let env = OSApp::EnvironmentVariable( group ).value_or( "" );
					setting = Str::Replace( setting, match, env );
				}
				value = setting;
			}
			else if( value.is_object() )
				SetEnv( value.get_object() );
		}
	}

	α Settings::Load()ι->void{
		let settingsPath = Path();
		try{
			LOG_MEMORY( {}, ELogLevel::Information, "current_path={}", std::filesystem::current_path() );
			if( !fs::exists(settingsPath) )
				throw std::runtime_error{ Ƒ("file does not exist: '{}'", settingsPath.string()) };
			_settings = mu<jvalue>( Json::ReadJsonNet(settingsPath) );
			if( _settings->is_object() )
				SetEnv( _settings->get_object() );
			LOG_MEMORY( {}, ELogLevel::Information, "Settings path={}", settingsPath.string() );
		}
		catch( const std::exception& e ){
			BREAK;
			_settings = mu<jvalue>( e.what() );
			LOG_MEMORY( {}, ELogLevel::Critical, "({})Could not load settings - {}", settingsPath.string(), e.what() );
		}
	}
	α Settings::Set( sv path, jvalue v, SL sl )ε->jvalue*{
		ASSERT( _settings );
		boost::system::error_code ec;
		auto y = _settings->set_at_pointer( path, move(v), ec );
		THROW_IFSL( ec, "Could not set '{}' to '{}'", path, serialize(v) );
		return y;
	}
}

/*	template<> α Find( sv path )ι->optional<T>{ return Value().Find<T>( path ); }
	template<class T=jobject> α FindPtr( sv path )ι->const T*{ return Value().FindPtr<T>( path ); }
	template<class T=sv> α FindDefault( sv path )ι->T{ return Value().FindDefault<T>(path); }
	template<class T> α FindEnum( sv path )ι->optional<T>{ return (T)Value().Find<uint>(path); }
*/
/*	template<> α Settings::FindPtr<JValue>( sv path )ι->optional<jvalue>{
		return Value().Find<JValue>( path );
	}
	template<> α Settings::Find<jobject>( sv path )ι->optional<jobject>{
		auto p = Find<jvalue>( path );
		return p && p->is_string() ? optional<jobject>{ p->as_object() } : optional<jobject>{};
	}
	template<> α Settings::Find<jarray>( sv path )ι->optional<jarray>{
		auto p = Find<jvalue>( path );
		return p && p->is_array() ? optional<string>{ p->as_array() } : optional<jarray>{};
	}

	template<> α Settings::Find<string>( sv path )ι->optional<string>{
		auto p = Find<jvalue>( path );
		return p && p->is_string() ? optional<string>{ p->as_string() } : optional<string>{};
	}
	template<> α Settings::Find<Duration>( sv path )ι->optional<Duration>{
		let duration = Find<string>( path );
		optional<std::chrono::system_clock::duration> result;
		if( duration.has_value() )
			Try( [duration, &result](){ result = Chrono::ToDuration(*duration);} );
		return result;
	}


	template<> α Settings::Find<uint>( sv path )ι->optional<uint>{
		auto p = Find<jvalue>( path );
		return p && p->is_uint64() ? optional<uint>{ p->as_uint64() } : optional<uint>{};
	}
	template<> α Settings::Find<uint8>( sv path )ι->optional<uint>{ return (uint8)Find<uint>( path ); }
	template<> α Settings::Find<uint16>( sv path )ι->optional<uint>{ return (uint16)Find<uint>( path ); }
	template<> α Settings::Find<uint32>( sv path )ι->optional<uint>{ return (uint32)Find<uint>( path ); }
*/
//}

/*

	Container::Container( const fs::path& jsonFile, SL sl )ε:
		_pJson{ mu<nlohmann::json>() }{
		CHECK_PATH( jsonFile, sl );
		let fileString = jsonFile.string();
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
		let values = Str::Split( path, '/' );
		auto j = _pJson.get();
		optional<json> result;
		for( uint i=0; i<values.size(); ++i ){
			let value = values[i];
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
		let path = Path();
		IO::FileUtilities::Save( path, j.dump(), std::ios_base::out, sl );
		DBG( "({})Saved for '{}'", path.string(), what );
	}

	α Settings::Set( sv what, const Container::Variant& v, bool save, SL sl )ε->void{
		let paths = Str::Split( what, '/' );
		auto parent = &Global().Json();
		for( uint i=0; i<paths.size()-1; ++i ){
			let subPath = paths[i];
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
		let original = pOriginal==parent->end() ? "null" : pOriginal->dump();
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
				let env = OSApp::EnvironmentVariable( group, sl ).value_or( "" );
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
}*/