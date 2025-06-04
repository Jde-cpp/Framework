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
	function<optional<ELogTags>(sv)> _tagFromString;
	function<optional<string>(ELogTags)> _tagToString;

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
/*	α Logging::Tag( const std::span<const sv> tags )ι->vector<sp<LogTag>>{
		vector<sp<LogTag>> y;
		for( let tag : tags )
			y.emplace_back( Tag(tag) );
		return y;
	}
	α Logging::Tag( sv tag )ι->sp<LogTag>{
		auto iter = find_if( _configuredTags, [&](let& x){return x->Id==tag;} );
		return iter==_configuredTags.end() ? _configuredTags.emplace_back( ms<LogTag>(LogTag{string{tag}, DefaultLogLevel()}) ) : *iter;
	}
	α Logging::Tag( ELogTags tag )ι->sp<LogTag>{
		return Tag( ToString(tag) );//TODO handle multiple. socket.client.read
	}

	Ω setTag( sv tagName, vector<ELogTags>& existing, ELogLevel configLevel )ι->string{
		using namespace Logging;
		let tag = ToLogTags( tagName );
		if( !tag )
			return {};
		ELogLevel globalLevel{};
		_configuredLevels.visit( tag, [&](auto& m){globalLevel = m->second;} );
		bool change = false;
		if( auto p = find_if(existing.begin(), existing.end(), [&](let& x){return x.Id==tagName;}); p!=existing.end() ){
			if( change = p->Level != level; change )
				p->Level = level;
		}
		else if( (change = level!=ELogLevel::NoLog) )
			existing.push_back( LogTag{globalTag->Id, level} );
		//global tag holds the minimum log level.
		if( change ){
			auto minLevel = level==ELogLevel::NoLog ? ELogLevel::Critical : level;
			auto updateMinLevel = [&]( const vector<LogTag>& otherSinkTags ){
				if( auto p = find_if( otherSinkTags, [&](let& x){return x.Id==tagName;} ); p!=otherSinkTags.end() )
					minLevel = std::min( minLevel, p->Level==ELogLevel::NoLog ? ELogLevel::Critical : p->Level );
			};
			if( &existing!=&_fileLogLevels )
				updateMinLevel( _fileLogLevels );
			//for_each( External::Loggers(), [&](let& x){ if(&existing!=&x->Tags) updateMinLevel(x->Tags);} );
			globalTag->Level = minLevel;
		}
		return string{tagName};
	}
	*/
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
	if( auto parser = tagStrings.empty() ? _tagToString : nullptr; parser )
		tagStrings = { parser(tags).value_or("NotFound") };

	return tagStrings.empty() ? string{ELogTagStrings[0]} : Str::AddSeparators( tagStrings, "." );
}

α Jde::ToLogTags( sv name )ι->ELogTags{
	auto flags = Str::Split( name, '.' );
	ELogTags y{};
	for( let& tag : flags ){
		if( auto i = (ELogTags)std::distance( ELogTagStrings.begin(), find(ELogTagStrings, tag) ); i<(ELogTags)ELogTagStrings.size() )
			y |= (ELogTags)( 1ul<<(underlying(i)-1) );
		else if( auto parsed = _tagFromString ? _tagFromString(tag) : nullopt; parsed )
			y |= *parsed;
		else
			Warning( ELogTags::Settings, "Unknown tag '{}'", tag );
	}
	return y;
}

α Jde::TagFromString( function<optional<ELogTags>(sv)> tagFromString )ι->void{ _tagFromString = tagFromString; }
α Jde::TagToString( function<string(ELogTags)> tagToString )ι->void{ _tagToString = tagToString; }

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
/*
α Jde::ShouldTrace( sp<LogTag> pTag )ι->bool{
	//TODO go through all tags.
	return pTag->Level==ELogLevel::Trace;
}
*/
α Jde::ShouldTrace( ELogTags tags )ι->bool {
	return MinLevel( tags ) == ELogLevel::Trace;
}