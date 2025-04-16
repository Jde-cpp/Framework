#include <jde/framework/log/log.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#ifdef _MSC_VER
	#include <crtdbg.h>
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/msvc_sink.h>
#endif
#include <boost/lexical_cast.hpp>
#include <jde/framework/settings.h>
#include <jde/framework/str.h>

#define let const auto

namespace Jde{
	inline constexpr std::array<sv,7> ELogLevelStrings = { "Trace", "Debug", "Information", "Warning", "Error", "Critical", "None" };
}

α Jde::LogLevelStrings()ι->const std::array<sv,7>{ return ELogLevelStrings; }

α Jde::ToString( ELogLevel l )ι->string{
	return FromEnum( ELogLevelStrings, l );
}
α Jde::ToLogLevel( sv l )ι->ELogLevel{
	return ToEnum<ELogLevel>( ELogLevelStrings, l ).value_or( ELogLevel::Error );
}

namespace Jde::Logging{
	bool _logMemory{true};
	up<vector<Logging::ExternalMessage>> _pMemoryLog; shared_mutex MemoryLogMutex;//TODO make external logger.
	auto _pOnceMessages = mu<flat_map<uint,flat_set<string>>>(); std::shared_mutex OnceMessageMutex;
	constexpr ELogTags _tags{ ELogTags::Settings };
}

namespace Jde{
	using spdlog::level::level_enum;
	TimePoint _startTime = Clock::now(); //Logging::Proto::Status _status; mutex _statusMutex; TimePoint _lastStatusUpdate;
	TimePoint Logging::StartTime()ι{ return _startTime;}

	α Logging::DestroyLogger()->void{
		Trace( ELogTags::App, "Destroying Logger" );
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
		let& sinkSettings = Settings::FindDefaultObject( "/logging/sinks" );
		for( let& [name,sink] : sinkSettings ){
			spdlog::sink_ptr pSink;
			string additional;
			auto pattern =  Json::FindSV( sink, "pattern" );
			if( name=="console" && IApplication::IsConsole() ){
				if( !pattern ){
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
				std::cout << "file sink:" << serialize(sink) << std::endl;
				optional<fs::path> pPath;
				if( auto p = Json::FindString(sink, "/path"); p )
					pPath = fs::path{ *p };
#pragma warning(disable: 4127)
				if( !_msvc && pPath && pPath->string().starts_with("/Jde-cpp") )
					pPath = fs::path{ "~/."+pPath->string().substr(1) };
				let markdown = Json::FindBool(sink, "/md" ).value_or( false );
				let fileNameWithExt = Settings::FileStem()+( markdown ? ".md" : ".log" );
				let path = pPath && !pPath->empty() ? *pPath/fileNameWithExt : OSApp::ApplicationDataFolder()/"logs"/fileNameWithExt;
				let truncate = Json::FindBool( sink, "/truncate" ).value_or( true );
				additional = Ƒ( " truncate='{}' path='{}'", truncate, path.string() );
				try{
					pSink = ms<spdlog::sinks::basic_file_sink_mt>( path.string(), truncate );
				}
				catch( const spdlog::spdlog_ex& e ){
					LogMemoryDetail( Logging::Message{"settings", ELogLevel::Error, "Could not create log:  ({}) path='{}' - {}"}, name, path.string(), e.what() );
					std::cerr << Ƒ( "Could not create log:  ({}) path='{}' - {}", name, path.string(), path.string(), e.what() ) << std::endl;
					continue;
				}
				if( !pattern )
					pattern = markdown ? "%^%3!l%$-%H:%M:%S.%e [%v](%g#L%#)\\" : "%^%3!l%$-%H:%M:%S.%e %-64@ %v";
			}
			else
				continue;
			pSink->set_pattern( string{*pattern} );
			let level = Json::FindEnum<ELogLevel>( sink, "/level", ToLogLevel ).value_or( ELogLevel::Trace );
			pSink->set_level( (level_enum)level );
			//std::cout << Ƒ( "({})level='{}' pattern='{}'{}", name, ToString(level), pattern, additional ) << std::endl;
			LogMemoryDetail( Logging::Message{"settings", ELogLevel::Information, "({})level='{}' pattern='{}'{}"}, name, ToString(level), *pattern, additional );
			sinks.push_back( pSink );
		}
		Logging::_logMemory = Settings::FindBool("/logging/memory").value_or( false );
		return sinks;
	}

	optional<vector<spdlog::sink_ptr>> _sinks;
	α Sinks()->const vector<spdlog::sink_ptr>&{
		if( !_sinks )
			_sinks = LoadSinks();
		return *_sinks;
	}

	void OnCloseLogger( spdlog::logger* pLogger )ι;
	optional<spdlog::logger> _logger2;
	α SpdLog()->spdlog::logger&{
		if( !_logger2 )
			_logger2 = { "my_logger", Sinks().begin(), Sinks().end() };
		return *_logger2;
	}

	α Logging::ClientMinLevel()ι->ELogLevel{ return (ELogLevel)SpdLog().level(); }

/*	void OnCloseLogger( spdlog::logger* pLogger )ι{
		static bool firstPass{ true };
		if( firstPass ){
			firstPass = false;
			INFOT( Logging::_logTag, "Closing logger '{}'", pLogger->name() );
			SpdLog().reset();//zero out during destruction.
		}
		else
			delete pLogger;
	}*/
	bool _initialized{};
	α Logging::Initialized()ι->bool{ return _initialized; }
	α Logging::Initialize()ι->void{
		//_status.set_starttime( (google::protobuf::uint32)Clock::to_time_t(_startTime) );

		let flushOn = Settings::FindEnum<ELogLevel>( "/logging/flushOn", ToLogLevel ).value_or( _debug ? ELogLevel::Debug : ELogLevel::Information );
		SpdLog().flush_on( (level_enum)flushOn );
		AddFileTags();

		let minSinkLevel = std::accumulate( Sinks().begin(), Sinks().end(), ELogLevel::Critical, [](ELogLevel min, auto& p){ return std::min((ELogLevel)p->level(), min);} );
		SpdLog().set_level( (level_enum)minSinkLevel );
		if( _pMemoryLog ){
			for( let& m : *_pMemoryLog ){
				if( !ShouldLog(m) )
					continue;
				let message = Str::ToString( m.MessageView, m.Args );
				if( Sinks().size() )
				SpdLog().log( spdlog::source_loc{m.File,(int)m.LineNumber,m.Function}, (level_enum)m.Level, message );
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
		_initialized = true;
		Information{ _tags, "log minLevel='{}' flushOn='{}'", ELogLevelStrings[(int8)minSinkLevel], ELogLevelStrings[(uint8)flushOn] };//TODO add flushon to Server
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
		if( SpdLog().level()!=(level_enum)client ){
			Information{ _tags, "Setting log level from '{}' to '{}'", ToString((ELogLevel)SpdLog().level()), ToString(client) };
			Settings::Set( "/logging/console/level", jvalue{ToString(client)} );
			Settings::Set( "/logging/file/level", jvalue{ToString(client)} );
			SpdLog().set_level( (level_enum)client );
		}
		External::SetMinLevel( external );
	}

/*	α Logging::SetStatus( const vector<string>& values )ι->void{
		{
			lg _{_statusMutex};
			_status.clear_details();
			for( let& value : values )
				_status.add_details( value );
		}
		let now = Clock::now();
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

	spdlog::logger* Logging::Default()ι{ return &SpdLog(); }

	α ClearMemoryLog()ι->void{
		unique_lock l{ Logging::MemoryLogMutex };
		Logging::_pMemoryLog = Logging::_logMemory ? mu<vector<Logging::ExternalMessage>>() : nullptr;
	}

	α FindMemoryLog( uint32 messageId )ι->vector<Logging::ExternalMessage>{
		return FindMemoryLog( [messageId](let& msg){ return msg.MessageId==messageId;} );
	}
	α FindMemoryLog( function<bool(const Logging::ExternalMessage&)> f )ι->vector<Logging::ExternalMessage>{
		sl l{ Logging::MemoryLogMutex };
		ASSERT( Logging::_pMemoryLog );
		vector<Logging::ExternalMessage> results;
		for_each( *Logging::_pMemoryLog, [&](let& msg){if( f(msg) ) results.push_back(msg);} );
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
		let logServer{ (m.Tags() & ELogTags::ExternalLogger)!=ELogTags::None };
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
