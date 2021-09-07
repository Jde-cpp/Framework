#include <jde/Log.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#ifdef _MSC_VER
	#include <crtdbg.h>
	#include <spdlog/spdlog.h>
	#include <spdlog/fmt/ostr.h>
	#include <spdlog/sinks/msvc_sink.h>
	//#include "server/EtwSink.h"
#else
//	#include "Lttng.h"
#endif
#include <boost/lexical_cast.hpp>
#include "log/server/ServerSink.h"
#include "Settings.h"
#define var const auto

namespace Jde
{
	ELogLevel _serverLogLevel{ ELogLevel::None };
	up<Logging::IServerSink> _pServerSink;
	void SetServerSink( up<Logging::IServerSink> p )noexcept{ _pServerSink=move(p); if( _pServerSink ) _serverLogLevel=Settings::TryGet<ELogLevel>("logging/server/level").value_or(ELogLevel::Error); }
	void SetServerLevel( ELogLevel serverLevel )noexcept{ _serverLogLevel=serverLevel; }

	namespace Logging
	{
		bool _logMemory{true};
		up<vector<Logging::Messages::Message>> _pMemoryLog; shared_mutex MemoryLogMutex;
		auto _pOnceMessages = make_unique<flat_map<uint,set<string>>>(); std::shared_mutex OnceMessageMutex;
	}

	void DestroyLogger()
	{
		TRACE( "Destroying Logger"sv );
		// {
		// 	unique_lock l{ Logging::MemoryLogMutex };
		// 	Logging::_pMemoryLog = nullptr;
		// }
		Logging::_pOnceMessages = nullptr;
		_pServerSink = nullptr;
	};

#define PREFIX unique_lock l{ MemoryLogMutex }; if( !_pMemoryLog ) _pMemoryLog = make_unique<vector<Logging::Messages::Message>>();
	α Logging::LogMemory( const Logging::MessageBase& m )noexcept->void
	{
		PREFIX
		_pMemoryLog->emplace_back( m );
	}
	α Logging::LogMemory( Logging::MessageBase&& m )noexcept->void
	{
		PREFIX
		_pMemoryLog->emplace_back( move(m) );
	}
	α Logging::LogMemory( Logging::Message2&& m, vector<string> values )noexcept->void
	{
		PREFIX
		//std::cout << m.GetType() << endl;
		_pMemoryLog->emplace_back( move(m), move(values) );
	}
	α Logging::LogMemory( Logging::MessageBase&& m, vector<string> values )noexcept->void
	{
		PREFIX
		//std::cout << m.GetType() << endl;
		_pMemoryLog->emplace_back( move(m), move(values) );
	}
	α Logging::LogMemory( const Logging::MessageBase& m, vector<string> values )noexcept->void
	{
		PREFIX
		//Logging::Message x{ m, move(vector<string>{values}) };
		_pMemoryLog->emplace_back( m, move(values) );
	}

#ifdef _MSC_VER
	bool ShouldLogMemory()noexcept{ return _logMemory; }
#endif
	TimePoint _startTime = Clock::now(); Logging::Proto::Status _status; mutex _statusMutex; TimePoint _lastStatusUpdate;
	vector<spdlog::sink_ptr> LoadSinks()noexcept
	{
		vector<spdlog::sink_ptr> sinks;
		var sinkSettings = Settings::TryMembers( "logging" );
		for( var& [name,sink] : sinkSettings )
		{
			spdlog::sink_ptr pSink;
			string additional;
			if( name=="console" && IApplication::IsConsole() )
				pSink = make_shared<spdlog::sinks::stdout_color_sink_mt>();
			else if( name=="file" )
			{
				auto pPath = sink.TryGet<fs::path>( "path" );
				var fileNameWithExt = Settings::FileStem()+".log";
				var path = pPath && !pPath->empty() ? *pPath/fileNameWithExt : OSApp::ApplicationDataFolder()/"logs"/fileNameWithExt;
				var truncate = sink.TryGet<bool>( "truncate" ).value_or( true );
				additional = format( " truncate='{}' path='{}'", truncate, path.string() );
				pSink = make_shared<spdlog::sinks::basic_file_sink_mt>( path.string(), truncate );
			}
			else
				continue;
			var pattern = sink.TryGet<string>( "pattern" ).value_or( "%H:%M:%S.%e %g:%# [%^%l%$] %v" );
			pSink->set_pattern( pattern );
			var level = sink.TryGet<ELogLevel>( "level" ).value_or( ELogLevel::Debug );
			pSink->set_level( (spdlog::level::level_enum)level );
			LOG_MEMORY( ELogLevel::Information, "({})Sink - level='{}' pattern='{}'{}", name, ToString(level), pattern, additional );
			sinks.push_back( pSink );
		}
		return sinks;
	}
	vector<spdlog::sink_ptr> _sinks = LoadSinks();
	// vector<spdlog::sink_ptr>::iterator StartIter(){ return LoadSinks().begin(); }
	// vector<spdlog::sink_ptr>::iterator EndIter(){ return LoadSinks().end(); }
	//spdlog::sink_ptr ConsolePtr()noexcept{ return make_shared<spdlog::sinks::stdout_color_sink_mt>(); }
	//spdlog::sink_ptr FilePtr()noexcept{ return make_shared<spdlog::sinks::basic_file_sink_mt>(GetLogPath().string(), true); }


	//spdlog::logger _logger{ GetLogPath().empty() ? spdlog::logger{ "my_logger", ConsolePtr() } : spdlog::logger{ "my_logger", {ConsolePtr(), FilePtr()} } };
	spdlog::logger _logger{ "my_logger", _sinks.begin(), _sinks.end() };

	α InitializeLogger()noexcept->void
	{
		_status.set_starttime( (google::protobuf::uint32)Clock::to_time_t(_startTime) );

		var minLevel = std::accumulate( _sinks.begin(), _sinks.end(), (uint8)ELogLevel::None, [](uint8 min, auto& p){return std::min((uint8)p->level(),min);} );
		_logger.set_level( (spdlog::level::level_enum)minLevel );
		var flushOn = Settings::TryGet<ELogLevel>( "logging/flushOn" ).value_or( ELogLevel::Information );
		_logger.flush_on( (spdlog::level::level_enum)flushOn );
		if( var p = Settings::TryGet<PortType>("logging/server/port"); p && *p )
			Try( []{ SetServerSink(make_unique<Logging::ServerSink>()); } );
		auto pServer = static_cast<Logging::ServerSink*>( _pServerSink.get() );
		for( var& m : *Logging::_pMemoryLog )
		{
 			using ctx = fmt::format_context;
    		vector<fmt::basic_format_arg<ctx>> args;
			for( var& a : m.Variables )
				args.push_back( fmt::detail::make_arg<ctx>(a) );
			try
			{
				_logger.log( spdlog::source_loc{m.File.data(),m.LineNumber,m.Function.data()}, (spdlog::level::level_enum)m.Level, fmt::vformat(m.MessageView, fmt::basic_format_args<ctx>{args.data(), (int)args.size()}) );
			}
			catch( const fmt::v8::format_error& e )
			{
				ERR( "{} - {}", m.MessageView, e.what() );
			}
			if( pServer && _serverLogLevel<=m.Level )
				pServer->Log( Logging::Messages::Message{m} );
		}
		if( !(Logging::_logMemory=Settings::TryGet<bool>("logging/memory").value_or(false)) )
			ClearMemoryLog();

		INFO( "settings path='{}'", Settings::Path() );
		INFO( "log  minLevel='{}' flushOn='{}'", ELogLevelStrings[minLevel], ELogLevelStrings[(uint8)flushOn] );
	}

	void Logging::LogServer( const MessageBase& m )noexcept
	{
		ASSERT( _pServerSink );
		_pServerSink->Log( m );
	}
	void Logging::LogServer( const MessageBase& m, vector<string>& values )noexcept
	{
		ASSERT( _pServerSink );
		_pServerSink->Log( m, values );
	}
	void Logging::LogServer( Messages::Message& m )noexcept
	{
		ASSERT( _pServerSink );
		_pServerSink->Log( m );
	}

/*		void LogEtw( const Logging::MessageBase& messageBase )
		{
#ifdef _MSC_VER
			//_pEtwSink->Log( messageBase );
#else
//			_pLttng->Log( messageBase );
#endif
		}
		void LogEtw( const Logging::MessageBase& messageBase, const vector<string>& values )
		{
#ifdef _MSC_VER
			//_pEtwSink->Log( messageBase, values );
#else
//			_pLttng->Log( messageBase, values );
#endif
		}
*/
	TimePoint Logging::StartTime()noexcept{ return _startTime;}
	void SendStatus()noexcept
	{
		lock_guard l{_statusMutex};
		vector<string> variables; variables.reserve( _status.details_size()+1 );
		_status.set_memory( IApplication::MemorySize() );
		ostringstream os;
		os << "Memory=" << _status.memory();
		for( int i=0; i<_status.details_size(); ++i )
			os << ";  " << _status.details(i);

		LOGS( ELogLevel::Trace, os.str() );
		if( _pServerSink )
			_pServerSink->SendStatus = true;
		_lastStatusUpdate = Clock::now();
	}
	void Logging::SetLogLevel( ELogLevel client, ELogLevel server )noexcept
	{
		_logger.set_level( (spdlog::level::level_enum)client );
		if( _pServerSink )
			_serverLogLevel = server;
		{
			lock_guard l{_statusMutex};
			_status.set_serverloglevel( (Proto::ELogLevel)server );
			_status.set_clientloglevel( (Proto::ELogLevel)client );
		}
		SendStatus();
	}
	void Logging::SetStatus( const vector<string>& values )noexcept
	{
		{
			lock_guard l{_statusMutex};
			_status.clear_details();
			for( var& value : values )
				_status.add_details( value );
		}
		var now = Clock::now();
		if( _lastStatusUpdate+10s<now )
			SendStatus();
	}
	up<Logging::Proto::Status> Logging::GetStatus()noexcept
	{
		lock_guard l{_statusMutex};
		return make_unique<Proto::Status>( _status );
	}

	bool Logging::ShouldLogOnce( const Logging::MessageBase& messageBase )
	{
		std::unique_lock l( OnceMessageMutex );
		auto& messages = _pOnceMessages->emplace( messageBase.FileId, set<string>{} ).first->second;
		return messages.emplace(messageBase.MessageView).second;
	}
	void Logging::LogOnce( Logging::MessageBase&& messageBase )
	{
		if( ShouldLogOnce(messageBase) )
			Log( move(messageBase) );
	}
	bool HaveLogger()noexcept{ return true; }
#ifdef _MSC_VER
	spdlog::logger* Logging::GetDefaultLogger()noexcept
	{
		if( !spLogger )
		{
			auto pFoo = new spdlog::sinks::msvc_sink_st();
			auto pSink = std::shared_ptr<spdlog::sinks::msvc_sink_st>( pFoo  );
			spLogger = std::make_shared<spdlog::logger>( "my_logger", pSink );
			pLogger = spLogger.get();
			pLogger->debug( fmt::format("Started log {}", "here") );
		}
		return pLogger;
	}
	inline Logging::IServerSink* Logging::GetServerSink()noexcept{ return _pServerSink.get(); }
/*	void SetServerSink( Logging::IServerSink* p )noexcept
	{
		_pServerSink = p;
	}*/
#endif
	void ClearMemoryLog()noexcept
	{
		unique_lock l{ Logging::MemoryLogMutex };
		Logging::_pMemoryLog = Logging::_logMemory ? make_unique<vector<Logging::Messages::Message>>() : nullptr;
	}
	vector<Logging::Messages::Message> FindMemoryLog( uint32 messageId )noexcept
	{
		shared_lock l{ Logging::MemoryLogMutex };
		ASSERT( Logging::_pMemoryLog );
		vector<Logging::Messages::Message>  results;
		for_each( Logging::_pMemoryLog->begin(), Logging::_pMemoryLog->end(), [messageId,&results](var& msg){if( msg.MessageId==messageId) results.push_back(msg);} );
		return results;
	}

/*	std::ostream& operator<<( std::ostream& os, const ELogLevel& value )
	{
		os << (spdlog::level::level_enum)value;
		return os;
	}
*/
	namespace Logging
	{
		MessageBase::MessageBase( sv message, ELogLevel level, sv file, sv function, int line )noexcept:
			Level{level},
			MessageId{ IO::Crc::Calc32(message) },//{IO::Crc::Calc32(message)},
			MessageView{ message },
			FileId{ IO::Crc::Calc32(file) },
			File{ file },
			FunctionId{ IO::Crc::Calc32(function) },
			Function{ function },
			LineNumber{ line }
		{
			if( level!=ELogLevel::Trace )
				Fields |= EFields::Level;
			if( message.size() )
				Fields |= EFields::Message | EFields::MessageId;
			if( File.size() )
				Fields |= EFields::File | EFields::FileId;
			if( Function.size() )
				Fields |= EFields::Function | EFields::FunctionId;
			if( LineNumber )
				Fields |= EFields::LineNumber;
		}

		Message2::Message2( const MessageBase& b )noexcept:
			MessageBase{ b },
			_fileName{ FileName(b.File) }
		{
			File = _fileName;
		}
		Message2::Message2( ELogLevel level, string message, sv file, sv function, int line )noexcept:
			MessageBase( message, level, file, function, line ),
			_fileName{ FileName(file) }
		{
			File = _fileName;
			_pMessage = make_unique<string>( move(message) );
			MessageView = *_pMessage;
		}
	}
}

/*std::ostream& operator<<( std::ostream& os, const std::optional<double>& value )
{
	if( value.has_value() )
		os << value.value();
	else
		os << "null";
	return os;
}
*/