#include "ServerSink.h"
#include "../threading/Thread.h"
#include "../coroutine/Alarm.h"
#include "../DateTime.h"
#define var const auto

namespace Jde::Logging
{
	sp<Logging::IServerSink> _pServerSink;
	static var _logLevel{ Logging::TagLevel("log-server") };
	namespace Server
	{
		bool _enabled{ Settings::Get<PortType>("logging/server/port").value_or(0) };
		atomic<ELogLevel> _logLevel{ _enabled ? Settings::Get<ELogLevel>("logging/server/level").value_or(ELogLevel::Error) : ELogLevel::None };

#define OR(a,b) auto p=_pServerSink; return p ? p->a : b;
		α ApplicationId()ι->ApplicationPK{OR(ApplicationId(),0);}
		α InstanceId()ι->ApplicationInstancePK{OR(InstanceId(),0);}
		α IsLocal()ι->bool{ OR(IsLocal(),false); }

		α Level()ι->ELogLevel{ return _logLevel; }
		α SetLevel( ELogLevel x )ι->void{ _logLevel = x; }
		α Set( sp<Logging::IServerSink>&& p )ι->void{ _pServerSink = move(p); }
		α Destroy()ι->void{ _pServerSink = nullptr; }
#define IF(x) if( auto p=_pServerSink; p ) x; throw Exception("Not connected")
		α FetchSessionInfo( SessionPK sessionId )ε->SessionInfoAwait{ IF( return p->FetchSessionInfo(sessionId) ); }
		α Write( Logging::Proto::ToServer&& message )ε->void{ IF( p->Write(move(message)) ); }
		α Log( const Messages::ServerMessage& m )ι->void
		{
			if( auto p=_pServerSink; p )
				p->Log( m );
		}
		α Log( const Messages::ServerMessage& m, vector<string>& values )ι->void
		{
			if( auto p=_pServerSink; p )
				p->Log( m, values );
		}
		atomic_flag _sendStatus{};
		α SetSendStatus()ι->void{ _sendStatus.test_and_set(); }
		α WebSubscribe( ELogLevel level )ι->void{ if( auto p=_pServerSink; p )p->WebSubscribe( level ); }
 	}
	α ClientLevel()ι->ELogLevel{ return Settings::Get<ELogLevel>("logging/file/level").value_or(ELogLevel::Debug); }
/*	α Write( Logging::Proto::ToServer&& message )ι->void
	{
		if( auto p = _pServerSink; p )
			p->Write( t );
	}*/
}


namespace Jde::Logging
{
	flat_map<SessionPK, vector<HCoroutine>> _sessionInfoHandles; mutex _sessionInfoHandleMutex;

	IServerSink::~IServerSink()
	{
		LOGX( "~IServerSink" );
		Server::Set( nullptr );
		LOGX( "_pServerSink = nullptr" );
	}
	ServerSink::~ServerSink(){ LOGX("~ServerSink"); }
	α ServerSink::Create(bool wait/*=false*/)ι->Task
	{
		while( !_pServerSink )
		{
			if( wait )
				co_await Threading::Alarm::Wait( 5s );
			try
			{
				Server::Set( ms<ServerSink>() );
			}
			catch( const IException& e )
			{
				if( e.Code==2406168687 )//port=0
					break;
				wait = true;
			}
		}
	}

	ServerSink::ServerSink()ε:
		ProtoBase{ "logging/server", 0 }//,don't wan't default port, no-port=don't use
	{
		LOG( "ServerSink::ServerSink( Host='{}', Port='{}' )", Host, Port );
	}

	α ServerSink::OnConnected()ι->void{}


	α ServerSink::OnDisconnect()ι->void
	{
		Server::Set( nullptr );
		LOG( "Disconnected from LogServer."sv );
/*		_messagesSent.clear();
		_filesSent.clear();
		_functionsSent.clear();
		_usersSent.clear();
		_threadsSent.clear();
		_instanceId = 0;*/
		Create( true );
	}

	α ServerSink::OnReceive( Proto::FromServer& transmission )ι->void
	{
		for( auto i2=0; i2<transmission.messages_size(); ++i2 )
		{
			auto& item = *transmission.mutable_messages( i2 );
			if( item.has_strings() )
			{
				var& strings = item.strings();
				LOG( "received '{}' strings", strings.messages_size()+strings.files_size()+strings.functions_size()+strings.threads_size() );
				for( auto i=0; i<strings.messages_size(); ++i )
					_messagesSent.emplace( strings.messages(i) );
				for( auto i=0; i<strings.files_size(); ++i )
					_filesSent.emplace( strings.files(i) );
				for( auto i=0; i<strings.functions_size(); ++i )
					_functionsSent.emplace( strings.functions(i) );
				for( auto i=0; i<strings.threads_size(); ++i )
					_threadsSent.emplace( strings.threads(i) );
				_stringsLoaded = true;
			}
			else if( item.has_acknowledgement() )
			{
				var& ack = item.acknowledgement();
				Logging::LogNoServer( Logging::MessageBase("ServerSink::ServerSink( Host='{}', Port='{}' InstanceId={} )", ELogLevel::Information, MY_FILE, __func__, __LINE__), Host, Port, ack.instanceid() );

				auto p = mu<Proto::Instance>();
				p->set_application( _applicationName = IApplication::ApplicationName() );
				p->set_host( IApplication::HostName() );
				p->set_pid( (int32)OSApp::ProcessId() );
				p->set_server_log_level( (Logging::Proto::ELogLevel)Logging::Server::Level() );
				p->set_client_log_level( (Logging::Proto::ELogLevel)Logging::ClientLevel() );
				p->set_start_time( (google::protobuf::uint32)Clock::to_time_t(Logging::StartTime()) );
				p->set_rest_port( Settings::Get<PortType>("rest/port").value_or(0) );
				p->set_websocket_port(Settings::Get<PortType>("websocket/port").value_or(0) );
				var defaultInstanceName = _debug ? "debug" : "release";
				p->set_instance_name(Settings::Get<string>("instance").value_or(defaultInstanceName) );

				Proto::ToServer t;
				t.add_messages()->set_allocated_instance( p.release() );
				base::Write( move(t) );
				_instanceId = ack.instanceid();
			}
			else if( item.has_loglevels() )
			{
				var& levels = item.loglevels();
				Logging::SetLogLevel( (ELogLevel)levels.client(), (ELogLevel)levels.server() );
				INFO_ONCE( "'{}' started at '{}'"sv, _applicationName, ToIsoString(Logging::StartTime()) );
			}
			else if( item.has_custom() )
			{
				if( _customFunction )
					_customFunction( item.mutable_custom()->requestid(), move(*item.mutable_custom()->mutable_message()) );
				else
					ERR_ONCE( "No custom function set" );
			}
			else if( item.has_session_info() )
			{
				var& info = item.session_info();
				lg _{_sessionInfoHandleMutex};
				if( auto p = _sessionInfoHandles.find(info.session_id()); p!=_sessionInfoHandles.end() )
				{
					for( auto h : p->second )
						h.promise().get_return_object().SetResult( mu<Proto::SessionInfo>(info) );
					_sessionInfoHandles.erase( info.session_id() );
				}
			}
		}
	}

	up<Proto::RequestString> ToRequestString( Proto::EFields field, uint32 id, sv text )
	{
		auto pResult = mu<Proto::RequestString>();
		pResult->set_field( field );
		pResult->set_id( id );
		pResult->set_value( text.data(), text.size() );

		return pResult;
	}
	α ServerSink::SendCustom( uint32 requestId, const std::string& bytes )ι->void
	{
		auto pCustom = mu<Proto::CustomMessage>();
		pCustom->set_requestid( requestId );
		pCustom->set_message( bytes );

		Proto::ToServer t;
		t.add_messages()->set_allocated_custom( pCustom.release() );
		base::Write( move(t) );
	}

	α ServerSink::Write( const MessageBase& m, TimePoint time, vector<string>* pValues )ι->void
	{
		auto pProto = mu<Proto::Message>();
		pProto->set_allocated_time( new google::protobuf::Timestamp{IO::Proto::ToTimestamp(time)} );
		pProto->set_level( (Proto::ELogLevel)m.Level );
		pProto->set_messageid( (uint32)m.MessageId );
		pProto->set_fileid( (uint32)m.FileId );
		pProto->set_functionid( (uint32)m.FunctionId );
		pProto->set_linenumber( (uint32)m.LineNumber );
		pProto->set_userid( (uint32)m.UserId );
		pProto->set_threadid( m.ThreadId );
		if( pValues )
		{
			for( auto& value : *pValues )
				pProto->add_variables( move(value) );
		}
		auto processMessage = [&m, this]( Proto::ToServer& t )
		{
			if( _messagesSent.emplace(m.MessageId) )
				t.add_messages()->set_allocated_string( ToRequestString(Proto::EFields::MessageId, (uint32)m.MessageId, m.MessageView).release() );
			if( _filesSent.emplace(m.FileId) )
				t.add_messages()->set_allocated_string( ToRequestString(Proto::EFields::FileId, (uint32)m.FileId, m.File).release() );
			if( _functionsSent.emplace(m.FunctionId) )
				t.add_messages()->set_allocated_string( ToRequestString(Proto::EFields::FunctionId, (uint32)m.FunctionId, m.Function).release() );
		};

		if( _stringsLoaded )
		{
			Proto::ToServer t;
			{
				AtomicGuard l{ _bufferMutex };
				if( _buffer.messages_size() )
				{
					for( auto i=0; i<_buffer.messages_size(); ++i )
					{
						auto pMessage = _buffer.mutable_messages( i );
						if( pMessage->has_message() )
							t.add_messages()->set_allocated_message( pMessage->release_message() );
						else if( pMessage->has_string() )
						{
							auto pString = up<Logging::Proto::RequestString>( pMessage->release_string() );
							UnorderedSet<ID>& set = pString->field()==Proto::EFields::MessageId ? _messagesSent : pString->field()==Proto::EFields::FileId ? _filesSent : _functionsSent;
							if( !set.contains(pString->id()) )
								t.add_messages()->set_allocated_string( pString.release() );
						}
					}
					_buffer = Proto::ToServer{};
				}
			}
			processMessage( t );
			t.add_messages()->set_allocated_message( pProto.release() );
			if( Server::_sendStatus.test() )
			{
				Server::_sendStatus.clear();
				t.add_messages()->set_allocated_status( GetStatus().release() );
			}
			base::Write( move(t) );
		}
		else
		{
			AtomicGuard l{ _bufferMutex };
			_buffer.add_messages()->set_allocated_message( pProto.release() );;
			processMessage( _buffer );
		}
	}

	α SessionInfoAwait::await_suspend( HCoroutine h )ι->void
	{
		IAwait::await_suspend( h );
		{
			lg _{ _sessionInfoHandleMutex };
			_sessionInfoHandles[_sessionId].push_back( h );
		}
		auto m=mu<Proto::RequestSessionInfo>(); m->set_session_id(_sessionId);
		Proto::ToServer t;
		auto u = t.add_messages(); u->set_allocated_session_info( m.release() );
		Logging::Server::Write( move(t) );
	};
	namespace Messages
	{
		ServerMessage::ServerMessage( const MessageBase& base, vector<string> values )ι:
			Message{ base },
			Timestamp{ Clock::now() },
			Variables{ move(values) }
		{
			ThreadId = Threading::GetThreadId();
			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}

		ServerMessage::ServerMessage( const ServerMessage& rhs ):
			Message{ rhs },
			Timestamp{ rhs.Timestamp },
			Variables{ rhs.Variables },
			_pFunction{ rhs._pFunction ? mu<string>(*rhs._pFunction) : nullptr }
		{
			if( _pFunction )
				Function = _pFunction->c_str();
		}
	}
}