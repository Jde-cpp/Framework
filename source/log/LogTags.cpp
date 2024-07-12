#include <jde/log/LogTags.h>
#include <jde/log/IExternalLogger.h>
#include <jde/Str.h>
#define var const auto

namespace Jde{
	inline constexpr std::array<sv,20> ELogTagStrings = {"none",
		"sql", "exception", "users", "tests", "app", "settings", "io", "locks",
		"alarm", "cache", "sessions", "http", "socket", "client", "server", "sent",
		"received", "startup", "shutdown"};

	up<vector<sp<LogTag>>> _availableTags;
	vector<LogTag> _fileTags;
	optional<ELogLevel> _defaultLogLevel;
	α DefaultLogLevel()ι->ELogLevel{
		if( !_defaultLogLevel ) //may not be set when tags are initialized.
			_defaultLogLevel = Settings::Get<ELogLevel>("logging/defaultLevel").value_or(ELogLevel::Information);
		return *_defaultLogLevel;
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
		return Tag( ToString(tag) );//TODO handle multiple.
	}

	α SetTag( sv tag, vector<LogTag>& existing, ELogLevel configLevel )ι->string{
		using namespace Logging;
		var log{ tag[0]!='_' };
		var tagName = log ? tag : tag.substr(1);
		var globalTag = find_if( *_availableTags, [&](var& x){return x->Id==tagName;} );
		if( globalTag==_availableTags->end() ){
			LOG_MEMORY( "settings", ELogLevel::Error, "unknown tag '{}'", tagName );
			static auto showed{ false };
			if( !showed ){
				showed = true;
				ostringstream os;
				for_each( *_availableTags, [&](var p){ os << p->Id << ", ";} );
				LOG_MEMORY( "settings", ELogLevel::Debug, "available tags:  {}", os.str().substr(0, os.str().size()-2) );
			}
			return {};
		}
		var level = log ? configLevel : DefaultLogLevel();

		bool change = false;
		if( auto p = find_if(existing.begin(), existing.end(), [&](var& x){return x.Id==tagName;}); p!=existing.end() ){
			if( change = p->Level != level; change )
				p->Level = level;
		}
		else if( (change = level!=ELogLevel::NoLog) )
			existing.push_back( LogTag{(*globalTag)->Id, level} );
		//global tag holds the minimum log level.
		if( change ){
			auto minLevel = level==ELogLevel::NoLog ? ELogLevel::Critical : level;
			auto updateMinLevel = [&]( const vector<LogTag>& otherSinkTags ){
				if( auto p = find_if( otherSinkTags, [&](var& x){return x.Id==tagName;} ); p!=otherSinkTags.end() )
					minLevel = std::min( minLevel, p->Level==ELogLevel::NoLog ? ELogLevel::Critical : p->Level );
			};
			if( &existing!=&_fileTags )
				updateMinLevel( _fileTags );
			for_each( External::Loggers(), [&](var& x){ if(&existing!=&x->Tags) updateMinLevel(x->Tags);} );
			(*globalTag)->Level = minLevel;
		}
		return (*globalTag)->Id;
	}

	α Logging::AddTags( vector<LogTag>& sinkTags, sv path )ι->void{
		try{
			uint i=0;
			for( var& level : ELogLevelStrings ){
				var levelTags = Settings::TryArray<string>( Jde::format("{}/{}", path, level) );
				vector<string> tagIds;
				for( var& tag : levelTags )
					tagIds.push_back( SetTag(tag, sinkTags, (ELogLevel)i) );
				++i;
				if( tagIds.size() )
					LOG_MEMORY( "settings", ELogLevel::Information, &sinkTags==&_fileTags ? "({})FileTags:  {}." : "({})ServerTags:  {}.", level, Str::AddCommas(tagIds) );
			}
		}
		catch( const json::exception& e ){
			LOG_MEMORY( "settings", ELogLevel::Error, e.what() );
		}
	}
	α Logging::AddFileTags()ι->void{
		AddTags( _fileTags, "logging/tags" );
	}
	α Logging::ShouldLog( const ExternalMessage& m )ι->bool{
		auto p = find_if( _fileTags, [&](var& x){return x.Id==m.Tag;} );
		return p!=_fileTags.end() && p->Level<=m.Level;
	}
}
α Jde::ToString( ELogTags tags )ι->string{
	if( tags==ELogTags::None )
		return string{ELogTagStrings[0]};
	vector<sv> tagStrings;
	for( uint i=1; i<ELogTagStrings.size(); ++i ){
		if( (uint)tags & (1<<(i-1)) )
			tagStrings.push_back( ELogTagStrings[i] );
	}
	return tagStrings.empty() ? string{ELogTagStrings[0]} : Str::AddSeparators( tagStrings, "." );
}
α Jde::ToELogTags( str name )ι->ELogTags{
	auto flags = Str::Split( name, '.' );
	uint y{};
	for( var& tag : flags ){
		if( uint i = std::distance( ELogTagStrings.begin(), find(ELogTagStrings, tag) ); i<ELogTagStrings.size() )
			y |= y | (1<<(i-1));
	}
	return (ELogTags)y;
}

α Jde::ShouldTrace( sp<LogTag> pTag )ι->bool{
	//TODO go through all tags.
	return pTag->Level==ELogLevel::Trace;
}
