#include <jde/framework/log/logTags.h>
#include <jde/framework/log/IExternalLogger.h>
#include <jde/framework/settings.h>
#include <jde/framework/str.h>
#define let const auto

namespace Jde{
	constexpr std::array<sv,29> ELogTagStrings = {"none",
		"access", "app", "cache", "client", "crypto", "dbDriver", "exception", "externalLogger",
		"http", "io", "locks", "parsing", "pedantic", "ql", "read", "scheduler",
		"server", "sessions", "settings", "shutdown", "socket", "sql", "startup", "subscription",
		"test", "threads", "write"
	};

	concurrent_flat_map<ELogTags, ELogLevel> _configuredLevels;
	concurrent_flat_map<ELogTags,ELogLevel> _fileLogLevels;
	optional<ELogLevel> _defaultFileLogLevel;
	vector<up<Logging::ITagParser>> _tagParsers;
	α Logging::AddTagParser( up<ITagParser>&& tagParser )ι->void{ _tagParsers.emplace_back( std::move(tagParser) ); }

	α DefaultLogLevel()ι->ELogLevel{
		if( !_defaultFileLogLevel ) //may not be set when tags are initialized.
			_defaultFileLogLevel = Settings::FindEnum<ELogLevel>( "/logging/defaultLevel", ToLogLevel ).value_or( ELogLevel::Information );
		return *_defaultFileLogLevel;
	}

α toString( const concurrent_flat_map<ELogTags, ELogLevel>& settings )->string{
	flat_map<ELogLevel, vector<string>> levels;
	settings.cvisit_all( [&]( let& kv ){
		levels.try_emplace( kv.second, vector<string>{} ).first->second.push_back( ToString(kv.first) );
	});
	string y;
	for( auto& [level, tags] : levels )
		y += Ƒ( "[{}]: {}\n", FromEnum(LogLevelStrings(), level), Str::Join(tags, ",") );
	if( y.size() )
		y.pop_back();
  return y;
}

α Logging::AddTags( concurrent_flat_map<ELogTags,ELogLevel>& sinkTags, sv path )ι->void{
		uint i=0;
		for( let& level : LogLevelStrings() ){
			let levelTags = Settings::FindDefaultArray( Ƒ("/{}/{}", path, Str::ToLower(level)) );
			for( let& jtag : levelTags ){
				if( let tagName = jtag.try_as_string(); tagName ){
					let tag = ToLogTags( *tagName );
					if( !empty(tag) )
						sinkTags.emplace( tag, (ELogLevel)i );
				}
			}
			++i;
		}
//		if( settingLog.size() )
//			LOG_MEMORY( ELogTags::Settings, ELogLevel::Information, &sinkTags == &_fileLogLevels ? "FileTags:  {}." : "ServerTags:  {}.", toString(sinkTags) );
	}
	α Logging::TagSettings( string /*name*/, str path )ι->concurrent_flat_map<ELogTags,ELogLevel>{
		uint i=0;
		concurrent_flat_map<ELogTags,ELogLevel> y;
		for( let& level : LogLevelStrings() ){
			let levelTags = Settings::FindDefaultArray( Ƒ("{}/{}", path, level) );
			for( let& v : levelTags ){
				if( let name = v.is_string() ? v.as_string() : sv{}; !name.empty() ){
					if( let tag = ToLogTags( name ); !empty(name) )
						y.emplace( tag, (ELogLevel)i );
				}
			}
			++i;
		}
		return y;
	}

	α Logging::AddFileTags()ι->void{
		AddTags( _fileLogLevels, "logging/tags" );
		Information( ELogTags::Settings, "File tags:  {}", toString(_fileLogLevels) );
	}
	α Logging::ShouldLog( const ExternalMessage& m )ι->bool{
		return Min( m.Tags, _fileLogLevels ).value_or(ELogLevel::Critical) <= m.Level;
	}
}

#pragma warning(disable:4334)
α Jde::ToString( ELogTags tags )ι->string{
	if( tags==ELogTags::None )
		return string{ELogTagStrings[0]};
	vector<string> tagStrings;
	for( uint i=1; i<ELogTagStrings.size(); ++i ){
		if( (uint)tags & (uint)(1ul<<(i-1ul)) )
			tagStrings.push_back( string{ELogTagStrings[i]} );
	}
	for( let& parser : _tagParsers ){
		auto additional = parser->ToString(tags);
		if( additional.size() )
			tagStrings.push_back( additional );
	}
	return tagStrings.empty() ? string{ELogTagStrings[0]} : Str::Join( tagStrings, "." );
}

α Jde::ToLogTags( sv name )ι->ELogTags{
	auto flags = Str::Split( name, "." );
	ELogTags y{};
	for( let& subName : flags ){
		ELogTags tag{};
		if( auto i = (ELogTags)std::distance(ELogTagStrings.begin(), find(ELogTagStrings, subName)); i<(ELogTags)ELogTagStrings.size() )
			tag |= (ELogTags)( 1ul<<(underlying(i)-1) );
		else{
			for( uint i=0; i<_tagParsers.size() && empty(tag); ++i )
				tag |= _tagParsers[i]->ToTag( string{subName} );
		}
		if( empty(tag) )
			Warning( ELogTags::Settings, "Unknown tag '{}'", subName );
		y |= tag;
	}
	return y;
}

α Jde::FileMinLevel( ELogTags tags )ι->ELogLevel{
	return Min( tags, _fileLogLevels ).value_or( DefaultLogLevel() );
}

using enum Jde::ELogLevel;
α Jde::MinLevel( ELogTags tags )ι->ELogLevel{
	return Min( FileMinLevel(tags), Logging::External::MinLevel(tags) );
}

α Jde::Min( ELogLevel a, ELogLevel b )ι->ELogLevel{
	return a==NoLog || b==NoLog ? std::max( a, b ) : std::min( a, b );
}

α Jde::Min( ELogTags tags, const concurrent_flat_map<ELogTags,ELogLevel>& settings )ι->optional<ELogLevel>{
	optional<ELogLevel> min;
	if( tags!=ELogTags::Exception && !empty(tags & ELogTags::Exception) ){
		min = Min( ELogTags::Exception, settings );
		if( min )
			return min;
		tags = tags & ~ELogTags::Exception;
	}

	if( !settings.cvisit(tags, [&](let& kv){min = kv.second;}) ){
		for( uint i=1; i<ELogTagStrings.size(); ++i ){
			let flag = (ELogTags)( 1ul<<(i-1) );
			if( !empty(tags & flag) ){
				settings.cvisit( flag, [&](let& kv){
					min = min ? Min( *min, kv.second ) : kv.second;
				});
			}
		}
	}
	return min;
}

α Jde::ShouldTrace( ELogTags tags )ι->bool {
	return MinLevel( tags ) == ELogLevel::Trace;
}