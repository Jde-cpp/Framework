#include <jde/Log.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#ifdef _MSC_VER
	#include <crtdbg.h>
	#include <spdlog/spdlog.h>
	#include <spdlog/fmt/ostr.h>
	#include <spdlog/sinks/msvc_sink.h>
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
		array<Tag_,20> _tags;
		array<Tag_,20> _serverTags;

		α Tags()noexcept->const array<Tag_,20>&{return _tags;}
		α ServerTags()noexcept->const array<Tag_,20>&{return _serverTags;}
		bool _logMemory{true};
		up<vector<Logging::Messages::Message>> _pMemoryLog; shared_mutex MemoryLogMutex;
		auto _pOnceMessages = make_unique<flat_map<uint,set<string>>>(); std::shared_mutex OnceMessageMutex;

		flat_map<sv,vector<function<void(ELogLevel)>>> _tagCallbacks;
		α TagLevel( sv tag )noexcept->ELogLevel
		{
			return (ELogLevel)std::max( (int8)Internal::TagLevel(tag,_tags), (int8)Internal::TagLevel(tag,_serverTags) );
		}
		α TagLevel( sv tag, function<void(ELogLevel)> onChange, ELogLevel dflt )noexcept->ELogLevel
		{
			_tagCallbacks.try_emplace( tag ).first->second.emplace_back( onChange );
			ELogLevel l = TagLevel(tag);
			return l==ELogLevel::NoLog ? dflt : l;
		}
	}

	void DestroyLogger()
	{
		TRACE( "Destroying Logger"sv );

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
		_pMemoryLog->emplace_back( move(m), move(values) );
	}
	α Logging::LogMemory( Logging::MessageBase&& m, vector<string> values )noexcept->void
	{
		PREFIX
		_pMemoryLog->emplace_back( move(m), move(values) );
	}
	α Logging::LogMemory( const Logging::MessageBase& m, vector<string> values )noexcept->void
	{
		PREFIX
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
			var pattern = sink.TryGet<string>( "pattern" ).value_or( "%^%3!l%$-%H:%M:%S.%e %-64@ %v" );
			pSink->set_pattern( pattern );
			var level = sink.TryGet<ELogLevel>( "level" ).value_or( ELogLevel::Debug );
			pSink->set_level( (spdlog::level::level_enum)level );
			LOG_MEMORY( ELogLevel::Information, "({})level='{}' pattern='{}'{}", name, ToString(level), pattern, additional );
			sinks.push_back( pSink );
		}
		return sinks;
	}
	vector<spdlog::sink_ptr> _sinks = LoadSinks();

	α AddTags( array<Tag_,20>& tags, sv path )noexcept->void
	{
		auto l = ELogLevel::Debug;
		var settings = Settings::TryArray<string>( path );
		for( var& tag : settings )
		{
			bool change = false;
			if( auto p = find_if(tags.begin(), tags.end(), [tag](var x){return x.Id[0]==0 || tag==x.Id.data();}); p!=tags.end() && p->Id[0]!=0 )
			{
				if( l!=ELogLevel::None )
				{
					change = p->Level!=l;
					p->Level=l;
				}
				else
				{
					for( auto prev=p, next=std::next(p); next!=tags.end() && !prev->Empty(); ++next, ++prev )
					{
						memcpy( prev->Id.data(), next->Id.data(), next->Id.size() );
						prev->Level = next->Level;
					}
				}
			}
			else if( (change = l!=ELogLevel::None) )
			{
				auto p = find_if( tags.begin(), tags.end(), [](var x){return x.Empty();} );
				if( p==tags.end() )
					--p;
				memcpy( p->Id.data(), tag.data(), std::min(tag.size(), p->Id.size()) );
				if( tag.size()<p->Id.size() )
					p->Id[tag.size()] = '\0';
				p->Level=l;
			}
			if( change )
			{
				if( auto pCallback=Logging::_tagCallbacks.find(tag); pCallback!=Logging::_tagCallbacks.end() )
				{
					for( auto&& c : pCallback->second )
						c( l );
				}
			}
		}
	}

	spdlog::logger _logger{ "my_logger", _sinks.begin(), _sinks.end() };

	α Logging::Initialize()noexcept->void
	{
		AddTags( _tags, "logging/tags" );
		_status.set_starttime( (google::protobuf::uint32)Clock::to_time_t(_startTime) );

		_logMemory = Settings::TryGet<bool>("logging/memory").value_or( false );
		var minLevel = std::accumulate( _sinks.begin(), _sinks.end(), (uint8)ELogLevel::None, [](uint8 min, auto& p){return std::min((uint8)p->level(),min);} );
		_logger.set_level( (spdlog::level::level_enum)minLevel );
		var flushOn = Settings::TryGet<ELogLevel>( "logging/flushOn" ).value_or( ELogLevel::Information );
		_logger.flush_on( (spdlog::level::level_enum)flushOn );
		auto pServer = IServerSink::Enabled() ? ServerSink::Create() : nullptr;
		if( pServer )
			AddTags( _tags, "logging/server/tags" );
		for( var& m : *_pMemoryLog )
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
				pServer->Log( Messages::Message{m} );
		}
		if( !_logMemory )
			ClearMemoryLog();

		INFO( "settings path={}", Settings::Path() );
		INFO( "log minLevel='{}' flushOn='{}'", ELogLevelStrings[minLevel], ELogLevelStrings[(uint8)flushOn] );//TODO add flushon to Server
	}

	void Logging::LogServer( const MessageBase& m )noexcept
	{
		ASSERT( _pServerSink && m.Level!=ELogLevel::NoLog );
		_pServerSink->Log( m );
	}
	void Logging::LogServer( const MessageBase& m, vector<string>& values )noexcept
	{
		ASSERT( _pServerSink && m.Level!=ELogLevel::NoLog );
		_pServerSink->Log( m, values );
	}
	void Logging::LogServer( Messages::Message& m )noexcept
	{
		ASSERT( _pServerSink && m.Level!=ELogLevel::NoLog );
		_pServerSink->Log( m );
	}

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
		if( _logger.level()!=(spdlog::level::level_enum)client )
		{
			INFO( "Setting log level from '{}' to '{}'", Str::FromEnum(ELogLevelStrings, _logger.level()), Str::FromEnum(ELogLevelStrings, client) );
			//TODO remvoe comment _logger.set_level( (spdlog::level::level_enum)client );
		}
		if( _pServerSink )
			_serverLogLevel = server;
		{
			lock_guard l{_statusMutex};
			_status.set_serverloglevel( (Proto::ELogLevel)server );
			_status.set_clientloglevel( (Proto::ELogLevel)_logger.level() );
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

	bool Logging::ShouldLogOnce( const Logging::MessageBase& messageBase )noexcept
	{
		std::unique_lock l( OnceMessageMutex );
		auto& messages = _pOnceMessages->emplace( messageBase.FileId, set<string>{} ).first->second;
		return messages.emplace(messageBase.MessageView).second;
	}
	void Logging::LogOnce( Logging::MessageBase&& messageBase )noexcept
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
