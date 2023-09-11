#include "ServerSink.h"
#include "../threading/Thread.h"
#include "../DateTime.h"
#define var const auto

namespace Jde::Logging
{
	ELogLevel _serverLogLevel{ ELogLevel::None };
	up<Logging::IServerSink> _pServerSink;
	static var _logLevel{ Logging::TagLevel("log-server") };
}

namespace Jde
{
	α Logging::Server()ι->up<Logging::IServerSink>&{ return _pServerSink; }
	α Logging::SetServer( up<Logging::IServerSink> p )ι->void{ _pServerSink=move(p); if( _pServerSink ) _serverLogLevel=Settings::Get<ELogLevel>("logging/server/level").value_or(ELogLevel::Error); }
	α Logging::ServerLevel()ι->ELogLevel{ return _serverLogLevel; }
	α Logging::SetServerLevel( ELogLevel serverLevel )ι->void{ _serverLogLevel=serverLevel; }
	α Logging::ClientLevel()ι->ELogLevel{ return Settings::Get<ELogLevel>("logging/file/level").value_or(ELogLevel::Debug); }
}

namespace Jde::Logging
{
	flat_map<SessionPK, vector<HCoroutine>> _sessionInfoHandles; mutex _sessionInfoHandleMutex;

	bool IServerSink::_enabled{ Settings::Get<PortType>("logging/server/port").value_or(0)!=0 };
	IServerSink::~IServerSink()
	{
		LOGX( "~IServerSink" );
		SetServer( nullptr );
		LOGX( "_pServerSink = nullptr" );
	}
	ServerSink::~ServerSink(){ LOGX("~ServerSink"); }
	α ServerSink::Create()ι->ServerSink*
	{
		try
		{
			SetServer( mu<ServerSink>() );
		}
		catch( const IException& )
		{}
		return (ServerSink*)_pServerSink.get();
	}

	ServerSink::ServerSink()ι(false):
		ProtoBase{ "logging/server", 0 }//,don't wan't default port, no-port=don't use
	{
		LOG( "ServerSink::ServerSink( Host='{}', Port='{}' )", Host, Port );
	}

	α ServerSink::OnConnected()ι->void{}


	α ServerSink::OnDisconnect()ι->void
	{
		LOG( "Disconnected from LogServer."sv );
		_messagesSent.clear();
		_filesSent.clear();
		_functionsSent.clear();
		_usersSent.clear();
		_threadsSent.clear();
		_instanceId = 0;
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
				p->set_server_log_level( (Logging::Proto::ELogLevel)Logging::ServerLevel() );
				p->set_client_log_level( (Logging::Proto::ELogLevel)Logging::ClientLevel() );
				p->set_start_time( (google::protobuf::uint32)Clock::to_time_t(Logging::StartTime()) );
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
				lock_guard _{_sessionInfoHandleMutex};
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
		base::Write( t );
	}

	α ServerSink::Write( const MessageBase& m, TimePoint time, vector<string>* pValues )ι->void
	{
		auto pProto = mu<Proto::Message>();
		pProto->set_allocated_time( IO::Proto::ToTimestamp(time).release() );
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
			if( SendStatus.exchange(false) )
				t.add_messages()->set_allocated_status( GetStatus().release() );
			base::Write( t );
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
			lock_guard _{ _sessionInfoHandleMutex };
			_sessionInfoHandles[_sessionId].push_back( h );
		}
		auto m=mu<Proto::RequestSessionInfo>(); m->set_session_id(_sessionId);
		Proto::ToServer t;
		auto u = t.add_messages(); u->set_allocated_session_info( m.release() );
		((ProtoBase*)Server().get())->Write( t );
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