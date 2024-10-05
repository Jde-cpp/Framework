#include <jde/log/Log.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#ifdef _MSC_VER
	#include <crtdbg.h>
	#include <spdlog/spdlog.h>
//	#include <spdlog/fmt/ostr.h>
	#include <spdlog/sinks/msvc_sink.h>
#endif
#include <boost/lexical_cast.hpp>
#include "../Settings.h"

#define var const auto

namespace Jde::Logging{
	bool _logMemory{true};
	up<vector<Logging::ExternalMessage>> _pMemoryLog; shared_mutex MemoryLogMutex;//TODO make external logger.
	auto _pOnceMessages = mu<flat_map<uint,flat_set<string>>>(); std::shared_mutex OnceMessageMutex;
	sp<LogTag> _statusTag = Logging::Tag( "status" );
	static sp<LogTag> _logTag = Logging::Tag( "settings" );
}
namespace Jde{
	using spdlog::level::level_enum;
	TimePoint _startTime = Clock::now(); //Logging::Proto::Status _status; mutex _statusMutex; TimePoint _lastStatusUpdate;
	TimePoint Logging::StartTime()ι{ return _startTime;}

	α Logging::DestroyLogger()->void{
		TRACE( "Destroying Logger"sv );
		Logging::_pOnceMessages = nullptr;
		{
			unique_lock l{ MemoryLogMutex };
			_pMemoryLog = nullptr;
			_logMemory = false;
		}
		External::DestroyLoggers();
	};

#define PREFIX unique_lock l{ MemoryLogMutex }; if( !_pMemoryLog ) _pMemoryLog = mu<vector<Logging::ExternalMessage>>();
	α Logging::LogMemory( const Logging::MessageBase& m )ι->void{
		PREFIX
		_pMemoryLog->emplace_back( move(m) );
	}

	α Logging::LogMemory( Logging::Message&& m, vector<string> values )ι->void{
		PREFIX
		_pMemoryLog->emplace_back( move(m), move(values) );
	}

	α Logging::LogMemory( const Logging::MessageBase& m, vector<string> values )ι->void{
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
#pragma warning(disable: 4127)
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
			pSink->set_level( (level_enum)level );
			//std::cout << Jde::format( "({})level='{}' pattern='{}'{}", name, ToString(level), pattern, additional ) << std::endl;
			LogMemoryDetail( Logging::Message{"settings", ELogLevel::Information, "({})level='{}' pattern='{}'{}"}, name, ToString(level), pattern, additional );
			sinks.push_back( pSink );
		}
		Logging::_logMemory = Settings::Get<bool>("logging/memory").value_or( false );
		return sinks;
	}
	vector<spdlog::sink_ptr> _sinks = LoadSinks();

	void OnCloseLogger( spdlog::logger* pLogger )ι;
	//using LoggerPtr = up<spdlog::logger, decltype(&Jde::OnCloseLogger)>;
	//auto _logger = LoggerPtr{ new spdlog::logger{"my_logger", _sinks.begin(), _sinks.end()}, Jde::OnCloseLogger };
	spdlog::logger _logger{ "my_logger", _sinks.begin(), _sinks.end() };
	α Logging::ClientMinLevel()ι->ELogLevel{ return (ELogLevel)_logger.level(); }

/*	void OnCloseLogger( spdlog::logger* pLogger )ι{
		static bool firstPass{ true };
		if( firstPass ){
			firstPass = false;
			INFOT( Logging::_logTag, "Closing logger '{}'", pLogger->name() );
			_logger.reset();//zero out during destruction.
		}
		else
			delete pLogger;
	}*/

	α Logging::Initialize()ι->void{
		//_status.set_starttime( (google::protobuf::uint32)Clock::to_time_t(_startTime) );

		var flushOn = Settings::Get<ELogLevel>( "logging/flushOn" ).value_or( _debug ? ELogLevel::Debug : ELogLevel::Information );
		_logger.flush_on( (level_enum)flushOn );
		AddFileTags();

		var minSinkLevel = std::accumulate( _sinks.begin(), _sinks.end(), ELogLevel::Critical, [](ELogLevel min, auto& p){ return std::min((ELogLevel)p->level(), min);} );
		_logger.set_level( (level_enum)minSinkLevel );
		if( _pMemoryLog ){
			for( var& m : *_pMemoryLog ){
				if( !ShouldLog(m) )
					continue;
				var message = Str::ToString( m.MessageView, m.Args );
				if( _sinks.size() )
					_logger.log( spdlog::source_loc{m.File,(int)m.LineNumber,m.Function}, (level_enum)m.Level, message );
				else
					std::cerr << message << std::endl;
				//no use case for loggers being initialized before this.
				// for_each( _externalLoggers, [&](auto& x){
				// 	if( x->MinLevel()<=m.Level )
				// 		x->Log( ExternalMessage{m} );
				// });
			}
			if( !_logMemory )
				ClearMemoryLog();
		}
		INFO( "settings path={}", Settings::Path().string() );
		INFO( "log minLevel='{}' flushOn='{}'", ELogLevelStrings[(int8)minSinkLevel], ELogLevelStrings[(uint8)flushOn] );//TODO add flushon to Server
	}


/*	α SendStatus()ι->void
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
	}*/
	α Logging::SetLogLevel( ELogLevel client, ELogLevel external )ι->void{
		if( _logger.level()!=(level_enum)client ){
			INFO( "Setting log level from '{}' to '{}'", Str::FromEnum(ELogLevelStrings, _logger.level()), Str::FromEnum(ELogLevelStrings, client) );
			Settings::Set( "logging/console/level", string{ToString(client)}, false );
			Settings::Set( "logging/file/level", string{ToString(client)} );
			_logger.set_level( (level_enum)client );
		}
		External::SetMinLevel( external );
	}

/*	α Logging::SetStatus( const vector<string>& values )ι->void{
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
*/
	α Logging::ShouldLogOnce( const Logging::MessageBase& messageBase )ι->bool
	{
		std::unique_lock l( OnceMessageMutex );
		auto& messages = _pOnceMessages->emplace( messageBase.FileId, flat_set<string>{} ).first->second;
		return messages.emplace(messageBase.MessageView).second;
	}
	α Logging::LogOnce( Logging::MessageBase&& m, const sp<LogTag>& tag )ι->void{
		if( ShouldLogOnce(m) )
			Log( move(m), tag, true );
	}

	spdlog::logger* Logging::Default()ι{ return &_logger; }

	α ClearMemoryLog()ι->void{
		unique_lock l{ Logging::MemoryLogMutex };
		Logging::_pMemoryLog = Logging::_logMemory ? mu<vector<Logging::ExternalMessage>>() : nullptr;
	}
	vector<Logging::ExternalMessage> FindMemoryLog( uint32 messageId )ι{
		sl l{ Logging::MemoryLogMutex };
		ASSERT( Logging::_pMemoryLog );
		vector<Logging::ExternalMessage>  results;
		for_each( Logging::_pMemoryLog->begin(), Logging::_pMemoryLog->end(), [messageId,&results](var& msg){if( msg.MessageId==messageId) results.push_back(msg);} );
		return results;
	}
/*	α Logging::Log( const Logging::ILogEntry& m )ι->void{
	//	if( m.Level<tag->Level || m.Level==ELogLevel::NoLog || tag->Level==ELogLevel::NoLog )
	//		return;
		try{
			BREAK_IF( m.Message.empty() );
			if( auto p = Default(); p )
				p->log( m.Source(), (level_enum)m.Level(), m.Message() );
			else{
				if( m.Level>=ELogLevel::Error )
					std::cerr << m.Message() << std::endl;
				else
					std::cout << m.Message() << std::endl;
			}
			BREAK_IF( m.Level>=BreakLevel() );
		}
		catch( const fmt::format_error& e ){
			critical( "FormatException message:'{}', file:'{}', line:{}, error:'{}'", m.Format(), m.File(), m.LineNumber(), e.what() );
		}
		var logServer{ (m.Tags() & ELogTags::ExternalLogger)!=ELogTags::None };
		if( logServer || LogMemory() ){
			//vector<string> values; values.reserve( sizeof...(args) );
			//ToVec::Append( values, args... );
			if( LogMemory() )
				LogMemory( m );
			if( logServer )
				External::Log( m );
		}
	}
*/
}
