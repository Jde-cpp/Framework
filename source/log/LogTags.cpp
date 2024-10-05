#include <jde/log/LogTags.h>
#include <jde/log/IExternalLogger.h>
#include <jde/Str.h>
#define var const auto

namespace Jde{
	constexpr std::array<sv,27> ELogTagStrings = {"none",
		"app", "cache", "client", "exception", "externalLogger", "http", "io", "locks",
		"parsing", "pedantic", "ql", "read", "scheduler", "server", "sessions", "settings",
		"shutdown", "socket", "sql", "startup", "subscription", "test", "threads", "um",
		"write"
		};

	up<vector<sp<LogTag>>> _availableTags;
	vector<LogTag> _fileTags;
	concurrent_flat_map<ELogTags,ELogLevel> _fileLogLevels;
	optional<ELogLevel> _defaultFileLogLevel;
	function<optional<ELogTags>(sv)> _parser;
	α DefaultLogLevel()ι->ELogLevel{
		if( !_defaultFileLogLevel ) //may not be set when tags are initialized.
			_defaultFileLogLevel = Settings::Get<ELogLevel>( "logging/defaultLevel" ).value_or( ELogLevel::Information );
		return *_defaultFileLogLevel;
	}

	α Logging::Tag( const std::span<const sv> tags )ι->vector<sp<LogTag>>{
		vector<sp<LogTag>> y;
		for( var tag : tags )
			y.emplace_back( Tag(tag) );
		return y;
	}
	α Logging::Tag( sv tag )ι->sp<LogTag>{
		if( !_availableTags )
			_availableTags = mu<vector<sp<LogTag>>>();
		auto iter = find_if( *_availableTags, [&](var& x){return x->Id==tag;} );
		return iter==_availableTags->end() ? _availableTags->emplace_back( ms<LogTag>(LogTag{string{tag}, DefaultLogLevel()}) ) : *iter;
	}
	α Logging::Tag( ELogTags tag )ι->sp<LogTag>{
		return Tag( ToString(tag) );//TODO handle multiple. socket.client.read
	}

	α SetTag( sv tag, vector<LogTag>& existing, ELogLevel configLevel )ι->string{
		using namespace Logging;
		var log{ tag[0]!='_' };
		var tagName = log ? tag : tag.substr(1);
		sp<LogTag> globalTag;
		auto pglobalTag = find_if( *_availableTags, [&](var& x){return x->Id==tagName;} );
		if( pglobalTag==_availableTags->end() )
			globalTag = _availableTags->emplace_back( ms<LogTag>( string{tagName}, configLevel ) );
		 else
		 	globalTag = *pglobalTag;
/*		if( globalTag==_availableTags->end() ){
			LOG_MEMORY( "settings", ELogLevel::Error, "unknown tag '{}'", tagName );
			static auto showed{ false };
			if( !showed ){
				showed = true;
				ostringstream os;
				for_each( *_availableTags, [&](var p){ os << p->Id << ", ";} );
				LOG_MEMORY( "settings", ELogLevel::Debug, "available tags:  {}", os.str().substr(0, os.str().size()-2) );
			}
			return {};
		}*/
		var level = log ? configLevel : DefaultLogLevel();

		bool change = false;
		if( auto p = find_if(existing.begin(), existing.end(), [&](var& x){return x.Id==tagName;}); p!=existing.end() ){
			if( change = p->Level != level; change )
				p->Level = level;
		}
		else if( (change = level!=ELogLevel::NoLog) )
			existing.push_back( LogTag{globalTag->Id, level} );
		//global tag holds the minimum log level.
		if( change ){
			auto minLevel = level==ELogLevel::NoLog ? ELogLevel::Critical : level;
			auto updateMinLevel = [&]( const vector<LogTag>& otherSinkTags ){
				if( auto p = find_if( otherSinkTags, [&](var& x){return x.Id==tagName;} ); p!=otherSinkTags.end() )
					minLevel = std::min( minLevel, p->Level==ELogLevel::NoLog ? ELogLevel::Critical : p->Level );
			};
			if( &existing!=&_fileTags )
				updateMinLevel( _fileTags );
			//for_each( External::Loggers(), [&](var& x){ if(&existing!=&x->Tags) updateMinLevel(x->Tags);} );
			globalTag->Level = minLevel;
		}
		return string{tagName};
	}

	α Logging::AddTags( vector<LogTag>& sinkTags, sv path )ι->void{
		try{
			uint i=0;
			for( var& level : ELogLevelStrings ){
				var levelTags = Settings::TryArray<string>( Jde::format("{}/{}", path, level) );
				vector<string> tagIds;
				for( var& tag : levelTags ){
					tagIds.push_back( SetTag(tag, sinkTags, (ELogLevel)i) );
					_fileLogLevels.emplace( ToLogTags(tag), (ELogLevel)i );
				}
				++i;
				if( tagIds.size() )
					LOG_MEMORY( "settings", ELogLevel::Information, &sinkTags==&_fileTags ? "({})FileTags:  {}." : "({})ServerTags:  {}.", level, Str::AddCommas(tagIds) );
			}
		}
		catch( const json::exception& e ){
			LOG_MEMORY( "settings", ELogLevel::Error, e.what() );
		}
	}
	α Logging::TagSettings( string /*name*/, str path )ι->concurrent_flat_map<ELogTags,ELogLevel>{
		uint i=0;
		concurrent_flat_map<ELogTags,ELogLevel> y;
		for( var& level : ELogLevelStrings ){
			var levelTags = Settings::TryArray<string>( Jde::format("{}/{}", path, level) );
			for( var& tagString : levelTags ){
				if( var tag = ToLogTags( tagString ); !empty(tag) )
					y.emplace( tag, (ELogLevel)i );
			}
			++i;
		}
		return y;
	}

	α Logging::AddFileTags()ι->void{
		AddTags( _fileTags, "logging/tags" );
	}
	α Logging::ShouldLog( const ExternalMessage& m )ι->bool{
		auto p = find_if( _fileTags, [&](var& x){return x.Id==m.Tag;} );
		return p!=_fileTags.end() && p->Level<=m.Level;
	}
}

#pragma warning(disable:4334)
α Jde::ToString( ELogTags tags )ι->string{
	if( tags==ELogTags::None )
		return string{ELogTagStrings[0]};
	vector<sv> tagStrings;
	for( uint i=1; i<ELogTagStrings.size(); ++i ){
		if( (uint)tags & (uint)(1ul<<(i-1ul)) )
			tagStrings.push_back( ELogTagStrings[i] );
	}
	return tagStrings.empty() ? string{ELogTagStrings[0]} : Str::AddSeparators( tagStrings, "." );
}

α Jde::ToLogTags( str name )ι->ELogTags{
	auto flags = Str::Split( name, '.' );
	ELogTags y{};
	for( var& tag : flags ){
		if( auto i = (ELogTags)std::distance( ELogTagStrings.begin(), find(ELogTagStrings, tag) ); i<(ELogTags)ELogTagStrings.size() )
			y |= (ELogTags)( 1ul<<(underlying(i)-1) );
		else if( auto parsed = _parser ? _parser(tag) : nullopt; parsed )
			y |= *parsed;
		else
			Warning( ELogTags::Settings, "Unknown tag '{}'", tag );
	}
	return y;
}

α Jde::TagParser( function<optional<ELogTags>(sv)> parser )ι->void{
	_parser = parser;
}

α Jde::FileMinLevel( ELogTags tags )ι->ELogLevel{
	return Min( tags, _fileLogLevels ).value_or( *_defaultFileLogLevel );
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
	if( !settings.cvisit(tags, [&](var& kv){min = kv.second;}) ){
		for( uint i=1; i<sizeof(ELogTags)*8; ++i ){
			var flag = (ELogTags)( 1ul<<(i-1) );
			if( !empty(tags & flag) )
				settings.cvisit( flag, [&](var& kv){min = min ? Min(*min, kv.second) : kv.second;} );
		}
	}
	return min;
}
α Jde::ShouldTrace( sp<LogTag> pTag )ι->bool{
	//TODO go through all tags.
	return pTag->Level==ELogLevel::Trace;
}

α Jde::ShouldTrace( ELogTags tags )ι->bool {
	return MinLevel( tags ) == ELogLevel::Trace;
}