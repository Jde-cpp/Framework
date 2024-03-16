#include <jde/Log.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#ifdef _MSC_VER
	#include <crtdbg.h>
	#include <spdlog/spdlog.h>
//	#include <spdlog/fmt/ostr.h>
	#include <spdlog/sinks/msvc_sink.h>
#endif
#include <boost/lexical_cast.hpp>
#include "Settings.h"
#include "io/ServerSink.h"
#include "collections/Collections.h"

#define var const auto

namespace Jde::Logging
{
	vector<LogTag> _fileTags;
	vector<LogTag> _serverTags;
	up<vector<sp<LogTag>>> _availableTags;
	bool _logMemory{true};
	up<vector<Logging::Messages::ServerMessage>> _pMemoryLog; shared_mutex MemoryLogMutex;
	auto _pOnceMessages = mu<flat_map<uint,std::set<string>>>(); std::shared_mutex OnceMessageMutex;
	sp<LogTag> _statusTag = Logging::Tag( "status" );
	static sp<LogTag> _logTag = Logging::Tag( "settings" );
	const ELogLevel _breakLevel{ Settings::Get<ELogLevel>("logging/breakLevel").value_or(ELogLevel::Warning) };
	ELogLevel BreakLevel()ι{ return _breakLevel; }
	α ServerLevel()ι->ELogLevel{ return Server::Level(); }

	α SetTag( sv tag, vector<LogTag>& existing, ELogLevel defaultLevel=ELogLevel::Debug )ι->sv
	{
		var log{ tag[0]!='_' };
		var tagName = log ? tag : tag.substr(1);
		var pc = find_if( *_availableTags, [&](var& x){return x->Id==tagName;} );
		if( pc==_availableTags->end() )
		{
			ERR( "unknown tag '{}'", tagName );
			static auto showed{ false };
			if( !showed )
			{
				showed = true;
				ostringstream os;
				for_each( *_availableTags, [&](var p){ os << p->Id << ", ";} );
				INFO( "available tags:  {}", os.str().substr(0, os.str().size()-2) );
			}
			return {};
		}
		var level = log ? defaultLevel : ELogLevel::NoLog;

		bool change = false;
		if( auto p = find_if(existing.begin(), existing.end(), [&](var& x){return x.Id==tagName;}); p!=existing.end() )
		{
			if( change = p->Level != level; change )
				p->Level = level;
		}
		else if( (change = level!=ELogLevel::NoLog) )
			existing.push_back( LogTag{(*pc)->Id, level} );
		if( change )
		{
			var& otherTags = &existing==&_fileTags ? _serverTags : _fileTags;
			var p = find_if( otherTags.begin(), otherTags.end(), [&](var& x){return x.Id==tagName;} );
			auto minLevel = p==otherTags.end() ? level : std::min( level==ELogLevel::NoLog ? ELogLevel::Critical : level, p->Level==ELogLevel::NoLog ? ELogLevel::Critical : p->Level );
			//if( minLevel==ELogLevel::None )
			//	minLevel = ELogLevel::NoLog;
			(*pc)->Level = minLevel;
		}
		return (*pc)->Id;
	}

	α AddTags( vector<LogTag>& tags, sv path )ι->void{
		try{
			uint i=0;
			for( var& level : ELogLevelStrings ){
				var levelTags = Settings::TryArray<json>( Jde::format("{}/{}", path, level) );
				vector<sv> tagIds;
				for( var& tag : levelTags )
					tagIds.push_back( SetTag(tag.get<string>(), tags, (ELogLevel)i) );
				++i;
				if( tagIds.size() )
					LOG_MEMORY( "settings", ELogLevel::Information, &tags==&_fileTags ? "({})FileTags:  {}." : "({})ServerTags:  {}.", level, Str::AddCommas(tagIds) );
			}
		}
		catch( const json::exception& e ){
			LOG_MEMORY( "settings", ELogLevel::Error, e.what() );
		}
	}
}
namespace Jde
{
	TimePoint _startTime = Clock::now(); Logging::Proto::Status _status; mutex _statusMutex; TimePoint _lastStatusUpdate;

	α Logging::SetTag( sv tag, ELogLevel, bool file )ι->void{ SetTag( tag, file ? _fileTags : _serverTags ); }
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
		return iter==_availableTags->end() ? _availableTags->emplace_back( ms<LogTag>(LogTag{string{tag}}) ) : *iter;
	}

	α Logging::DestroyLogger()->void
	{
		TRACE( "Destroying Logger"sv );
		Logging::_pOnceMessages = nullptr;
		{
			unique_lock l{ MemoryLogMutex };
			_pMemoryLog = nullptr;
			_logMemory = false;
		}
		Server::Destroy();
	};

#define PREFIX unique_lock l{ MemoryLogMutex }; if( !_pMemoryLog ) _pMemoryLog = mu<vector<Logging::Messages::ServerMessage>>();
	α Logging::LogMemory( const Logging::MessageBase& m )ι->void
	{
		PREFIX
		_pMemoryLog->emplace_back( move(m) );
	}

	α Logging::LogMemory( Logging::Message&& m, vector<string> values )ι->void
	{
		PREFIX
		_pMemoryLog->emplace_back( move(m), move(values) );
	}

	α Logging::LogMemory( const Logging::MessageBase& m, vector<string> values )ι->void
	{
		PREFIX
		_pMemoryLog->emplace_back( m, move(values) );
	}

	α Logging::LogMemory()ι->bool{ return _logMemory; }

	α LoadSinks()ι->vector<spdlog::sink_ptr>{
		vector<spdlog::sink_ptr> sinks;
		var sinkSettings = Settings::TryMembers( "logging" );
		for( var& [name,sink] : sinkSettings ){
			spdlog::sink_ptr pSink;
			string additional;
			string pattern = sink.Get<string>( "pattern" ).value_or("");
			if( name=="console" && IApplication::IsConsole() ){
				if( pattern.empty() ){
					if constexpr( _debug ){
#ifdef _MSC_VER
							pattern = "\u001b]8;;file://%g\u001b\\%3!#-%3!l%$-%H:%M:%S.%e %v\u001b]8;;\u001b";
#else
						pattern = "%^%3!l%$-%H:%M:%S.%e \e]8;;file://%g#%#\a%v\e]8;;\a";
						//pattern = "%^%3!l%$-%H:%M:%S.%e %v %g#%#";//%-64@  %v
#endif
					}
					else
						pattern = "%^%3!l%$-%H:%M:%S.%e %v";//%-64@  %v
				}
				pSink = ms<spdlog::sinks::stdout_color_sink_mt>();
			}
			else if( name=="file" ){
				auto pPath = sink.Get<fs::path>( "path" );
				if( !_msvc && pPath && pPath->string().starts_with("/Jde-cpp") )
					pPath = fs::path{ "~/."+pPath->string().substr(1) };
				var markdown = sink.Get<bool>( "md" ).value_or( false );
				var fileNameWithExt = Settings::FileStem()+( markdown ? ".md" : ".log" );
				var path = pPath && !pPath->empty() ? *pPath/fileNameWithExt : OSApp::ApplicationDataFolder()/"logs"/fileNameWithExt;
				var truncate = sink.Get<bool>( "truncate" ).value_or( true );
				additional = Jde::format( " truncate='{}' path='{}'", truncate, path.string() );
				try{
					pSink = ms<spdlog::sinks::basic_file_sink_mt>( path.string(), truncate );
				}
				catch( const spdlog::spdlog_ex& e ){
					LogMemoryDetail( Logging::Message{"settings", ELogLevel::Error, "Could not create log:  ({}) path='{}' - {}"}, name, path.string(), e.what() );
					std::cerr << Jde::format( "Could not create log:  ({}) path='{}' - {}", name, path.string(), path.string(), e.what() ) << std::endl;
					continue;
				}
				if( pattern.empty() )
					pattern = markdown ? "%^%3!l%$-%H:%M:%S.%e [%v](%g#L%#)\\" : "%^%3!l%$-%H:%M:%S.%e %-64@ %v";
			}
			else
				continue;
			pSink->set_pattern( pattern );
			var level = sink.Get<ELogLevel>( "level" ).value_or( ELogLevel::Trace );
			pSink->set_level( (spdlog::level::level_enum)level );
			std::cout << Jde::format( "({})level='{}' pattern='{}'{}", name, ToString(level), pattern, additional ) << std::endl;
			LogMemoryDetail( Logging::Message{"settings", ELogLevel::Information, "({})level='{}' pattern='{}'{}"}, name, ToString(level), pattern, additional );
			sinks.push_back( pSink );
		}
		Logging::_logMemory = Settings::Get<bool>("logging/memory").value_or( false );
		return sinks;
	}
	vector<spdlog::sink_ptr> _sinks = LoadSinks();

	void OnCloseLogger( spdlog::logger* pLogger )ι;
	using LoggerPtr = up<spdlog::logger, decltype(&Jde::OnCloseLogger)>;
	auto _logger = LoggerPtr{ new spdlog::logger{"my_logger", _sinks.begin(), _sinks.end()}, Jde::OnCloseLogger };
	void OnCloseLogger( spdlog::logger* pLogger )ι{
		static bool firstPass{ true };
		if( firstPass ){
			firstPass = false;
			INFOT( Logging::_logTag, "Closing logger '{}'", pLogger->name() );
			_logger.reset();//zero out during destruction.
		}
		else
			delete pLogger;
	}

	α Logging::Initialize()ι->void{
		_status.set_starttime( (google::protobuf::uint32)Clock::to_time_t(_startTime) );

		var flushOn = Settings::Get<ELogLevel>( "logging/flushOn" ).value_or( _debug ? ELogLevel::Debug : ELogLevel::Information );
		_logger->flush_on( (spdlog::level::level_enum)flushOn );
		if( Server::Enabled() ){
			AddTags( _serverTags, "logging/server/tags" );
			ServerSink::Create();
		}
		AddTags( _fileTags, "logging/tags" );

		var minSinkLevel = std::accumulate( _sinks.begin(), _sinks.end(), ELogLevel::Critical, [](ELogLevel min, auto& p){ return std::min((ELogLevel)p->level(), min);} );
		for( auto tag : *_availableTags )
			tag->Level = tag->Level==ELogLevel::NoLog ? tag->Level : std::max( tag->Level, minSinkLevel );
		_logger->set_level( (spdlog::level::level_enum)minSinkLevel );
		if( _pMemoryLog ){
			for( var& m : *_pMemoryLog ){
				
 				using ctx = fmt::format_context;
    		vector<fmt::basic_format_arg<ctx>> args;
				for( var& a : m.Variables )
				 	args.push_back( fmt::detail::make_arg<ctx>(a) );
				try{
					if( m.Tag.size() && find_if(_fileTags.begin(), _fileTags.end(), [&](var& x){return x.Id==m.Tag;})==_fileTags.end() )
						continue;
					if( _sinks.size() )
						_logger->log( spdlog::source_loc{m.File,(int)m.LineNumber,m.Function}, (spdlog::level::level_enum)m.Level, fmt::vformat(m.MessageView, fmt::basic_format_args<ctx>{args.data(), (int)args.size()}) );
						//_logger->log( spdlog::source_loc{m.File,(int)m.LineNumber,m.Function}, (spdlog::level::level_enum)m.Level, ToVec::FormatVectorArgs(m.MessageView, m.Variables) );
					else
						std::cerr << ToVec::FormatVectorArgs(m.MessageView, m.Variables) << std::endl;
				}
				catch( const fmt::format_error& e ){
					ERR( "{} - {}", m.MessageView, e.what() );
				}
				if( Server::Enabled() && Server::Level()<=m.Level )
					Server::Log( Messages::ServerMessage{m} );
			}
			if( !_logMemory )
				ClearMemoryLog();
		}
		INFO( "settings path={}", Settings::Path().string() );
		INFO( "log minLevel='{}' flushOn='{}'", ELogLevelStrings[(int8)minSinkLevel], ELogLevelStrings[(uint8)flushOn] );//TODO add flushon to Server
	}

	α Logging::LogServer( const MessageBase& m )ι->void
	{
		ASSERTX( m.Level!=ELogLevel::NoLog );
		Server::Log( m );
	}
	α Logging::LogServer( const MessageBase& m, vector<string>& values )ι->void
	{
		ASSERTX( m.Level!=ELogLevel::NoLog );
		Server::Log( m, values );
	}
	α Logging::LogServer( Messages::ServerMessage& m )ι->void
	{
		ASSERTX( m.Level!=ELogLevel::NoLog );
		Server::Log( m );
	}

	TimePoint Logging::StartTime()ι{ return _startTime;}
	α SendStatus()ι->void
	{
		lg _{_statusMutex};
		vector<string> variables; variables.reserve( _status.details_size()+1 );
		_status.set_memory( IApplication::MemorySize() );
		ostringstream os;
		os << "Memory=" << _status.memory();
		for( int i=0; i<_status.details_size(); ++i )
			os << ";  " << _status.details(i);

		TRACET( Logging::_statusTag, "{}", os.str() );
		if( Logging::Server::Enabled() )
			Logging::Server::SetSendStatus();
		_lastStatusUpdate = Clock::now();
	}
	α Logging::SetLogLevel( ELogLevel client, ELogLevel server )ι->void
	{
		if( _logger->level()!=(spdlog::level::level_enum)client )
		{
			INFO( "Setting log level from '{}' to '{}'", Str::FromEnum(ELogLevelStrings, _logger->level()), Str::FromEnum(ELogLevelStrings, client) );
			Settings::Set( "logging/console/level", string{ToString(client)}, false );
			Settings::Set( "logging/file/level", string{ToString(client)} );
			//TODO remvoe comment _logger->set_level( (spdlog::level::level_enum)client );
		}
		Logging::Server::SetLevel( server );
		{
			lg _{_statusMutex};
			_status.set_serverloglevel( (Proto::ELogLevel)server );
			_status.set_clientloglevel( (Proto::ELogLevel)_logger->level() );
		}
		SendStatus();
	}
	α Logging::SetStatus( const vector<string>& values )ι->void
	{
		{
			lg _{_statusMutex};
			_status.clear_details();
			for( var& value : values )
				_status.add_details( value );
		}
		var now = Clock::now();
		if( _lastStatusUpdate+10s<now )
			SendStatus();
	}
	α Logging::GetStatus()ι->up<Logging::Proto::Status>
	{
		lg _{_statusMutex};
		return mu<Proto::Status>( _status );
	}

	α Logging::ShouldLogOnce( const Logging::MessageBase& messageBase )ι->bool
	{
		std::unique_lock l( OnceMessageMutex );
		auto& messages = _pOnceMessages->emplace( messageBase.FileId, set<string>{} ).first->second;
		return messages.emplace(messageBase.MessageView).second;
	}
	α Logging::LogOnce( Logging::MessageBase&& m, const sp<LogTag>& tag )ι->void{
		if( ShouldLogOnce(m) )
			Log( move(m), tag, true );
	}
	α HaveLogger()ι->bool{
		return _logger.get(); 
	}

	spdlog::logger* Logging::Default()ι{ return _logger.get(); }

	α ClearMemoryLog()ι->void
	{
		unique_lock l{ Logging::MemoryLogMutex };
		Logging::_pMemoryLog = Logging::_logMemory ? mu<vector<Logging::Messages::ServerMessage>>() : nullptr;
	}
	vector<Logging::Messages::ServerMessage> FindMemoryLog( uint32 messageId )ι
	{
		shared_lock l{ Logging::MemoryLogMutex };
		ASSERT( Logging::_pMemoryLog );
		vector<Logging::Messages::ServerMessage>  results;
		for_each( Logging::_pMemoryLog->begin(), Logging::_pMemoryLog->end(), [messageId,&results](var& msg){if( msg.MessageId==messageId) results.push_back(msg);} );
		return results;
	}


	namespace Logging
	{
		MessageBase::MessageBase( ELogLevel level, const source_location& sl )ι:
			Fields{ EFields::File | EFields::FileId | EFields::Function | EFields::FunctionId | EFields::LineNumber },
			Level{ level },
			MessageId{ 0 },//{},
			FileId{ Calc32RunTime(FileName(sl.file_name())) },
			File{ sl.file_name() },
			FunctionId{ Calc32RunTime(sl.function_name()) },
			Function{ sl.function_name() },
			LineNumber{ sl.line() }
		{
			if( level!=ELogLevel::Trace )
				Fields |= EFields::Level;
		}
		MessageBase::MessageBase( ELogLevel level, sv message, const char* file, const char* function, uint_least32_t line )ι:
			Fields{ EFields::Message | EFields::File | EFields::FileId | EFields::Function | EFields::FunctionId | EFields::LineNumber },
			Level{ level },
			MessageId{ Calc32RunTime(message) },//{},
			FileId{ Calc32RunTime(FileName(file)) },
			File{ file },
			FunctionId{ Calc32RunTime(function) },
			Function{ function },
			LineNumber{ line }
		{}


		Message::Message( const MessageBase& b )ι:
			MessageBase{ b },
			_fileName{ FileName(b.File) }
		{
			File = _fileName.c_str();
		}

		Message::Message( ELogLevel level, string message, const source_location& sl )ι:
			MessageBase( level, sl ),
			_pMessage{ mu<string>(move(message)) },
			_fileName{ FileName(sl.file_name()) }
		{
			File = _fileName.c_str();
			MessageView = *_pMessage;
			MessageId = Calc32RunTime( MessageView );
		}

		Message::Message( sv tag, ELogLevel level, string message, const source_location& sl )ι:
			MessageBase( level, sl ),
			Tag{ tag },
			_pMessage{ mu<string>(move(message)) },
			_fileName{ FileName(sl.file_name()) }
		{
			File = _fileName.c_str();
			MessageView = *_pMessage;
			MessageId = Calc32RunTime( MessageView );
		}

		Message::Message( sv tag, ELogLevel level, string message, char const* file, char const * function, boost::uint_least32_t line )ι:
			MessageBase{ level, message, file, function, line },
			Tag{ tag },
			_pMessage{ mu<string>(move(message)) },
			_fileName{ FileName(file) }
		{
			File = _fileName.c_str();
			MessageView = *_pMessage;
			MessageId = Calc32RunTime( MessageView );
			ASSERT( level>=ELogLevel::NoLog && level<=ELogLevel::Critical );
		}

		Message::Message( const Message& x )ι:
			MessageBase{ x },
			Tag{ x.Tag },
			_pMessage{ x._pMessage ? mu<string>(*x._pMessage) : nullptr },
			_fileName{ x._fileName }
		{
			File = _fileName.c_str();
			if( _pMessage )
			{
				MessageView = *_pMessage;
				MessageId = Calc32RunTime( MessageView );
			}
		}
	}
}