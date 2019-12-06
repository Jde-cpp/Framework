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

//_crtBreakAlloc = 155;

namespace Jde
{
	Logging::IServerSink* _pServerSink{nullptr};
	std::shared_ptr<spdlog::logger> spLogger{nullptr};
	spdlog::logger* pLogger{nullptr};
	namespace Logging
	{
		auto _pOnceMessages = make_unique<map<uint,set<uint>>>();
		std::shared_mutex OnceMessageMutex;
	}

	void DestroyLogger()
	{
		TRACE0( "Destroying Logger" ); 
		pLogger = nullptr; 
		spLogger = nullptr; 
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
		var server =  pSettings->Get<bool>( "server", false );
		InitializeLogger( level, path, server );
	}
	TimePoint _startTime = Clock::now(); Logging::Proto::Status _status; mutex _statusMutex; TimePoint _lastStatusUpdate;
	void SecretDelFunc( spdlog::logger* p) 
	{ 
		delete p;
		//spLogger = nullptr;
		pLogger = nullptr;
	}
	void InitializeLogger( ELogLevel level2, const fs::path& path, bool server )noexcept
	{
		//auto sinkx = spdlog::stdout_color_mt( "console" );
		_status.set_starttime( (google::protobuf::uint32)Clock::to_time_t(_startTime) );

		auto pConsole = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		if( !path.empty() )
		{
			//string fileName(  );
			auto pFileSink = make_shared<spdlog::sinks::basic_file_sink_mt>( path.string(), true );
			//auto pLogger2 = spdlog::basic_logger_mt( "basic_logger", string(fileName) );
			//auto rotating = make_shared<spdlog::sinks::rotating_file_sink_mt> ("logs/testlog", "log", 1024*1024, 5, false);
			//std::vector<spdlog::sink_ptr> sinks;
			std::vector<spdlog::sink_ptr> sinks{pConsole, pFileSink};
//			sinks.push_back( pConsole ); 
//			sinks.push_back( pFileSink );
			//std::shared_ptr<sinks::sink>
			
			spLogger = sp<spdlog::logger>( new spdlog::logger{"my_logger", sinks.begin(), sinks.end()}, SecretDelFunc );
			//spLogger = make_shared<spdlog::logger>( "my_logger", sinks.begin(), sinks.end() );
		}
		else
			spLogger = make_shared<spdlog::logger>( "my_logger", pConsole );
			//spLogger = spdlog::stdout_color_mt( "console" );
		pLogger = spLogger.get();
		pLogger->set_level( (spdlog::level::level_enum)level2 );
		pLogger->flush_on( spdlog::level::level_enum::info );
		if( server )
		{
			_pServerSink = Logging::ServerSink::Create( Diagnostics::HostName(), (uint16)4321 ).get();
#if _MSC_VER
			//_spEtwSink =  Logging::EtwSink::Create( boost::lexical_cast<boost::uuids::uuid>("1D6E1834-B684-4CA1-A5AC-ADA270FF8F24") );
			//_pEtwSink = _spEtwSink.get();
#else
//			_spLttng =  Logging::Lttng::Create();
//			_pLttng = _spLttng.get();
#endif
		}
		INFO( "Created log level:  {},  flush on:  {}", ELogLevelStrings[(uint)level2], ELogLevelStrings[(uint)ELogLevel::Information] );
		//DBG0( ""sv );
	}

	namespace Logging
	{
		void LogCritical( const Logging::MessageBase& messageBase )
		{
			GetDefaultLogger()->log( spdlog::level::level_enum::critical, messageBase.MessageView );
			if( GetServerSink() )
				LogServer( messageBase );
			// if( GetEtwSink() )
			// 	LogEtw( messageBase );
		}

		void LogServer( const Logging::MessageBase& messageBase )
		{
			if( _pServerSink->GetLogLevel()<=messageBase.Level )
				_pServerSink->Log( messageBase );
		}
		void LogServer( const Logging::MessageBase& messageBase, const vector<string>& values )
		{
			if( _pServerSink->GetLogLevel()<=messageBase.Level )
				_pServerSink->Log( messageBase, values );
		}
		void LogServer( const Logging::Messages::Message& message )
		{
			if( _pServerSink->GetLogLevel()<=message.Level )
				_pServerSink->Log( message );
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
			auto& lineIds = _pOnceMessages->emplace( messageBase.FileId, set<uint>{} ).first->second;
			return lineIds.emplace(messageBase.LineNumber).second;
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
	std::ostream& operator<<( std::ostream& os, const ELogLevel& value )
	{
		os << (spdlog::level::level_enum)value;
		return os;
	}

	namespace Logging
	{
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