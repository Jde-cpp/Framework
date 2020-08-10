#include "Logging.h"
#include "../Diagnostics.h"
#include <boost/lexical_cast.hpp>
//#include "../io/Buffer.h"
#include "server/ServerSink.h"
#include "../Settings.h"
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
#define var const auto

namespace Jde
{
	Logging::IServerSink* _pServerSink{nullptr};
	bool _logMemory{false};
	std::unique_ptr<std::vector<Logging::Messages::Message>> _pMemoryLog;
	std::shared_ptr<spdlog::logger> spLogger{nullptr};
	spdlog::logger* pLogger{nullptr};
	namespace Logging
	{
		auto _pOnceMessages = make_unique<map<uint,set<string>>>();
		std::shared_mutex OnceMessageMutex;
	}

	void DestroyLogger()
	{
		TRACE0( "Destroying Logger"sv );
		pLogger = nullptr;
		spLogger = nullptr;
		_pMemoryLog = nullptr;
		Logging::_pOnceMessages = nullptr;
		_pServerSink = nullptr;
	};

	//std::shared_ptr<Logging::ServerSink> _spServerSink2{nullptr};
	//std::shared_ptr<Logging::IServerSink> _spServerSink{nullptr};
#ifdef _MSC_VER
	//std::shared_ptr<Logging::EtwSink> _spEtwSink;
	//Logging::EtwSink* _pEtwSink;
	//Logging::EtwSink* GetEtwSink(){ return _pEtwSink;}
#else
//	Logging::Lttng* _pLttng;
//	sp<Logging::Lttng> _spLttng;
//	Logging::Lttng* GetEtwSink(){ return _pLttng;}
#endif

	void InitializeLogger( string_view fileName )noexcept
	{
		var pSettings = Settings::Global().SubContainer( "logging" );
		var level = (ELogLevel)pSettings->Get<int>( "level", (int)ELogLevel::Debug );
		auto path =  pSettings->Get<fs::path>( "path", fs::path() );
		if( fileName.size() && !path.empty() )
			path/=fmt::format( "{}.log", fileName );
		var serverPort =  pSettings->Get<uint16>( "serverPort", 0 );
		var memory =  pSettings->Get<uint16>( "memory", false );
		InitializeLogger( level, path, serverPort, memory );
	}
	TimePoint _startTime = Clock::now(); Logging::Proto::Status _status; mutex _statusMutex; TimePoint _lastStatusUpdate;
	void SecretDelFunc( spdlog::logger* p )
	{
		delete p;
		//spLogger = nullptr;
		pLogger = nullptr;
	}
	void InitializeLogger( ELogLevel level2, const fs::path& path, uint16 serverPort, bool memory )noexcept
	{
		//auto sinkx = spdlog::stdout_color_mt( "console" );
		_status.set_starttime( (google::protobuf::uint32)Clock::to_time_t(_startTime) );

		auto pConsole = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		if( !path.empty() )
		{
			auto pFileSink = make_shared<spdlog::sinks::basic_file_sink_mt>( path.string(), true );
			std::vector<spdlog::sink_ptr> sinks{pConsole, pFileSink};
			spLogger = sp<spdlog::logger>( new spdlog::logger{"my_logger", sinks.begin(), sinks.end()}, SecretDelFunc );
		}
		else
			spLogger = make_shared<spdlog::logger>( "my_logger", pConsole );
		pLogger = spLogger.get();
		pLogger->set_level( (spdlog::level::level_enum)level2 );
		pLogger->flush_on( spdlog::level::level_enum::info );
		if( serverPort )
		{
			_pServerSink = Logging::ServerSink::Create( Diagnostics::HostName(), (uint16)serverPort ).get();
#if _MSC_VER
			//_spEtwSink =  Logging::EtwSink::Create( boost::lexical_cast<boost::uuids::uuid>("1D6E1834-B684-4CA1-A5AC-ADA270FF8F24") );
			//_pEtwSink = _spEtwSink.get();
#else
//			_spLttng =  Logging::Lttng::Create();
//			_pLttng = _spLttng.get();
#endif
		}
		if( memory )
			ClearMemoryLog();
		INFO( "Created log level:  {},  flush on:  {}"sv, ELogLevelStrings[(uint)level2], ELogLevelStrings[(uint)ELogLevel::Information] );
		//DBG0( ""sv );
	}

	namespace Logging
	{
		void LogCritical( const Logging::MessageBase& messageBase )noexcept
		{
			GetDefaultLogger()->log( spdlog::level::level_enum::critical, messageBase.MessageView );
			if( GetServerSink() )
				LogServer( messageBase );
			// if( GetEtwSink() )
			// 	LogEtw( messageBase );
		}
		void LogServer( const Logging::MessageBase& messageBase )noexcept
		{
			if( _pServerSink && _pServerSink->GetLogLevel()<=messageBase.Level )
				_pServerSink->Log( messageBase );
		}
		void LogServer( const Logging::MessageBase& messageBase, const vector<string>& values )noexcept
		{
			if( _pServerSink && _pServerSink->GetLogLevel()<=messageBase.Level )
				_pServerSink->Log( messageBase, values );
		}
		void LogServer( const Logging::Messages::Message& message )noexcept
		{
			if( _pServerSink && _pServerSink->GetLogLevel()<=message.Level )
				_pServerSink->Log( message );
		}

		void LogMemory( const Logging::MessageBase& messageBase, const vector<string>* pValues )noexcept
		{
			if( _pMemoryLog )
			{
				if( pValues )
					_pMemoryLog->push_back( Logging::Messages::Message{messageBase, *pValues} );
				else
					_pMemoryLog->push_back( Logging::Messages::Message{messageBase} );
			}
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
		TimePoint StartTime()noexcept{ return _startTime;}
		void SendStatus()noexcept
		{
			lock_guard l{_statusMutex};
			vector<string> variables; variables.reserve( _status.details_size()+1 );
			_status.set_memory( Diagnostics::GetMemorySize() );
			ostringstream os;
			os << "Memory=" << _status.memory();
			for( int i=0; i<_status.details_size(); ++i )
				os << ";  " << _status.details(i);

			TRACE0X( os.str() );
			if( _pServerSink )
				_pServerSink->SendStatus = true;
			_lastStatusUpdate = Clock::now();
		}
		void SetLogLevel( ELogLevel client, ELogLevel server )noexcept
		{
			GetDefaultLogger()->set_level( (spdlog::level::level_enum)client );
			if( _pServerSink )
				_pServerSink->SetLogLevel( (ELogLevel)server );
			{
				lock_guard l{_statusMutex};
				_status.set_serverloglevel( (Proto::ELogLevel)server );
				_status.set_clientloglevel( (Proto::ELogLevel)client );
			}
			SendStatus();
		}
		void SetStatus( const vector<string>& values )noexcept
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
		Proto::Status* GetAllocatedStatus()noexcept
		{
			lock_guard l{_statusMutex};
			return new Proto::Status( _status );
		}

		bool ShouldLogOnce( const Logging::MessageBase& messageBase )
		{
			std::unique_lock l( OnceMessageMutex );
			auto& messages = _pOnceMessages->emplace( messageBase.FileId, set<string>{} ).first->second;
			return messages.emplace(messageBase.MessageView).second;
		}
		void LogOnce( const Logging::MessageBase& messageBase )
		{
			if( ShouldLogOnce(messageBase) )
				Log( messageBase );
		}
	}
	bool HaveLogger()noexcept{ return pLogger!=nullptr; }
#ifdef _MSC_VER
	spdlog::logger* GetDefaultLogger()noexcept
	{
		//static std::shared_ptr<spdlog::logger> spLogger = nullptr;
		//static spdlog::logger* pLogger = nullptr;
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
	inline Logging::IServerSink* GetServerSink()noexcept
	{
		return _pServerSink;
	}
	void SetServerSink( Logging::IServerSink* p )noexcept
	{
		_pServerSink = p;
	}
#endif
	void ClearMemoryLog()noexcept{ _logMemory = true; _pMemoryLog = std::make_unique<std::vector<Logging::Messages::Message>>(); }
	vector<Logging::Messages::Message> FindMemoryLog( uint32 messageId )noexcept
	{
		assert( _pMemoryLog );
		vector<Logging::Messages::Message>  results;
		for_each( _pMemoryLog->begin(), _pMemoryLog->end(), [messageId,&results](var& msg){if( msg.MessageId==messageId) results.push_back(msg);} );
		return results;
	}

	std::ostream& operator<<( std::ostream& os, const ELogLevel& value )
	{
		os << (spdlog::level::level_enum)value;
		return os;
	}

	namespace Logging
	{
		MessageBase::MessageBase( ELogLevel level, sp<string> pMessage, std::string_view file, std::string_view function, uint line )noexcept:
			MessageBase( level, *pMessage, file, function, line, IO::Crc::Calc32(*pMessage), IO::Crc::Calc32(file), IO::Crc::Calc32(function) )
		{
			_pMessage = pMessage;
		}

		MessageBase::MessageBase( ELogLevel level, const string& message, std::string_view file, std::string_view function, uint line )noexcept:
			MessageBase( level, message, file, function, line, IO::Crc::Calc32(message), IO::Crc::Calc32(file), IO::Crc::Calc32(function) )
		{
			_pMessage = make_shared<string>( message );
			MessageView = *_pMessage;
		}

/*		MessageBase::MessageBase( IO::IncomingMessage& message, EFields fields ):
			Fields{ fields },
			Level{ (fields & EFields::Level)!=EFields::None ? (ELogLevel)message.ReadUInt() : ELogLevel::Trace },
			MessageId{ (fields & EFields::MessageId)!=EFields::None ? message.ReadUInt() : 0 },
			FileId{ (fields & EFields::FileId)!=EFields::None ? message.ReadUInt() : 0 },
			FunctionId{ (fields & EFields::FunctionId)!=EFields::None ? message.ReadUInt() : 0 },
			LineNumber{ (fields & EFields::LineNumber)!=EFields::None ? message.ReadUInt() : 0 },
			UserId{ (fields & EFields::UserId)!=EFields::None ? message.ReadUInt() : 0 },
			ThreadId{ (fields & EFields::ThreadId)!=EFields::None ? message.ReadUInt() : 0 }
		{}
*/
		//set<Messages::Message> OnceMessages;
		//std::shared_mutex OnceMessageMutex;
/*		void LogOnceVec( const Logging::MessageBase& messageBase, const vector<string>& values )
		{
			bool log = false;
			Messages::Message message( messageBase, values );
			{
				std::shared_lock l( OnceMessageMutex );
				log = OnceMessages.emplace(message).second;
			}
			if( log )
			{
				pLogger->log( ( spdlog::level::level_enum)messageBase.Level, messageBase.MessageView.data(), args... );
				if( _pServerSink )
				{
					vector<string> values; values.reserve( sizeof...(args) );
					ToVec::Append( values, args... );
					LogServer( messageBase, values );
				}
			}
		}*/
	}
}

std::ostream& operator<<( std::ostream& os, const std::optional<double>& value )
{
	if( value.has_value() )
		os << value.value();
	else
		os << "null";
	return os;
}