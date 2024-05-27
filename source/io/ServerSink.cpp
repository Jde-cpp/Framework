#include "ServerSink.h"
#include <boost/unordered/concurrent_flat_map.hpp>
#include <jde/io/Json.h>
#include "../threading/Thread.h"
#include "../coroutine/Alarm.h"
#include "../DateTime.h"
#include "proto/ToServerMsg.h"
#define var const auto

namespace Jde::Logging{
	sp<Logging::IServerSink> _pServerSink;//IServerSink=ServerSink=TProtoClient=ProtoClient=ProtoClientSession
	static sp<LogTag> _logTag{ Tag("appServer") };
	//TODO when socket terminates, error these out.
	boost::concurrent_flat_map<uint,HCoroutine> _returnCalls;  //request_id

	namespace Server{
		bool _enabled{ Settings::Get<PortType>("logging/server/port").value_or(0)>0 };
		atomic<ELogLevel> _level{ _enabled ? Settings::Get<ELogLevel>("logging/server/level").value_or(ELogLevel::Error) : ELogLevel::NoLog };

#define OR(a,b) auto p=_pServerSink; return p ? p->a : b;
		α ApplicationId()ι->ApplicationPK{OR(ApplicationId(),0);}
		α InstanceId()ι->ApplicationInstancePK{OR(InstanceId(),0);}
		α IsLocal()ι->bool{ OR(IsLocal(),false); }

		α Level()ι->ELogLevel{ return _level; }
		α SetLevel( ELogLevel x )ι->void{ _level = x; }
		α Set( sp<Logging::IServerSink>&& p )ι->void{ _pServerSink = move(p); }
		α Destroy()ι->void{
			if( !_pServerSink )
				return;
			DBG( "Destroying ServerSink - use_count={}", _pServerSink.use_count() );
			_pServerSink->Close();
//			_pServerSink = nullptr;
		}
#define IF(x) if( auto p=_pServerSink; p ) x; throw Exception("Not connected")
		α FetchSessionInfo( SessionPK sessionId )ε->SessionInfoAwait{ IF( return p->FetchSessionInfo(sessionId) ); }
		α Write( Logging::Proto::ToServer&& message )ε->void{ IF( p->Write(move(message)) ); }
		α Log( const Messages::ServerMessage& m )ι->void{
			if( auto p=_pServerSink; p )
				p->Log( m );
		}
		α Log( const Messages::ServerMessage& m, vector<string>& values )ι->void{
			if( auto p=_pServerSink; p )
				p->Log( m, values );
		}
		atomic_flag _sendStatus{};
		α SetSendStatus()ι->void{ _sendStatus.test_and_set(); }
		α WebSubscribe( ELogLevel level )ι->void{ if( auto p=_pServerSink; p )p->WebSubscribe( level ); }
 	}
	α ClientLevel()ι->ELogLevel{ return Settings::Get<ELogLevel>("logging/file/level").value_or(ELogLevel::Debug); }
}


namespace Jde::Logging{
	flat_map<SessionPK, vector<HCoroutine>> _sessionInfoHandles; mutex _sessionInfoHandleMutex;

	IServerSink::IServerSink()ι:IServerSink{ {} }{}
	IServerSink::IServerSink( const unordered_set<ID>& msgs )ι:_messagesSent{msgs},_logTag{Logging::_logTag}{}

	IServerSink::~IServerSink(){
		TRACEX( "~IServerSink" );
		Server::Set( nullptr );
		TRACEX( "_pServerSink = nullptr" );
	}
	ServerSink::~ServerSink(){ TRACEX("~ServerSink - Application Socket"); }
	α ServerSink::Create(bool wait/*=false*/)ι->Task{
		DBGT( Logging::_logTag, "ServerSink::Create" );
		while( !_pServerSink ){
			if( wait )
				co_await Threading::Alarm::Wait( wait ? 5s : 0s );//ms asan issue for some reason when not hitting.
			if( IApplication::ShuttingDown() ){
				DBGT( Logging::_logTag, "ShuttingDown - Stop trying to connect to app server." );
				break;
			}
			try{
				auto p = ms<ServerSink>();
				p->Connect();
				Server::Set( move(p) );
			}
			catch( const IException& e ){
				if( e.Code==2406168687 )//port=0
					break;
				wait = true;
			}
		}
		DBGT( Logging::_logTag, "ServerSink::~Create" );
	}

	ServerSink::ServerSink()ε:
		ProtoBase{ "logging/server", 0 }{//don't wan't default port, no-port=don't use
		TRACE( "ServerSink::ServerSink( Host='{}', Port='{}' )", Host, Port );
	}

	α ServerSink::OnConnected()ι->void{}


	α ServerSink::OnDisconnect()ι->void{
		Server::Set( nullptr );
		TRACE( "Disconnected from LogServer."sv );
		vector<HCoroutine> handles;
		if( _returnCalls.erase_if([&handles]( auto& kv ){
			handles.push_back( kv.second );
			return true;
		} )){
			for( auto& h : handles )
				Resume( Exception{"Disconnected from LogServer"}, h );
		}
/*		_messagesSent.clear();
		_filesSent.clear();
		_functionsSent.clear();
		_usersSent.clear();
		_threadsSent.clear();
		_instanceId = 0;*/
		Create( true );
	}

	α ServerSink::OnReceive( Proto::FromServer& transmission )ι->void{
		for( auto i2=0; i2<transmission.messages_size(); ++i2 ){
			auto& item = *transmission.mutable_messages( i2 );
			if( item.has_strings() ){
				var& strings = item.strings();
				TRACE( "received '{}' strings", strings.messages_size()+strings.files_size()+strings.functions_size()+strings.threads_size() );
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
			else if( item.has_acknowledgement() ){
				var& ack = item.acknowledgement();
				LOGX( ELogLevel::Information, _logTag, "Connected to LogServer. Host='{}', Port='{}' InstanceId={} )", Host, Port, ack.instanceid() );
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
			else if( item.has_loglevels() ){
				var& levels = item.loglevels();
				Logging::SetLogLevel( (ELogLevel)levels.client(), (ELogLevel)levels.server() );
				LOG_ONCE( ELogLevel::Information, _logTag, "'{}' started at '{}'"sv, _applicationName, ToIsoString(Logging::StartTime()) );
			}
			else if( item.has_custom() ){
				if( _customFunction )
					_customFunction( item.mutable_custom()->requestid(), move(*item.mutable_custom()->mutable_message()) );
				else
					LOG_ONCE( ELogLevel::Error, _logTag, "No custom function set" );
			}
/*			else if( auto p = item.has_authenticate() ? item.mutable_authenticate() : nullptr; p ){
				if( _authenticateFunction )
					_authenticateFunction( p->session_id(), move(*p->mutable_user()), move(*p->mutable_password()), move(*p->mutable_opc_server()) );
				else
					LOG_ONCE( ELogLevel::Error, _logTag, "No authenticate function set." );
			}*/
			else if( item.has_session_info() ){
				var& info = item.session_info();
				lg _{_sessionInfoHandleMutex};
				if( auto p = _sessionInfoHandles.find(info.session_id()); p!=_sessionInfoHandles.end() ){
					for( auto h : p->second )
						h.promise().SetResult( mu<Proto::SessionInfo>(info) );
					_sessionInfoHandles.erase( info.session_id() );
				}
			}
			else if( auto p = item.has_add_session_result() ? item.mutable_add_session_result() : nullptr; p ){
				HCoroutine h{};
				if( _returnCalls.erase_if(p->request_id(), [sessionId=p->session_id(), &h]( auto& kv ){
					h = move( kv.second );
					h.promise().SetResult( mu<SessionPK>(sessionId) );
					return true;
				} )){
					h.resume();
				}
				else
					ERR( "({})No AddLoginAwait found.", p->request_id() );
			}
			else if( auto p = item.has_graph_ql() ? item.mutable_graph_ql() : nullptr; p ){
				HCoroutine h{};
				if( _returnCalls.erase_if(p->request_id(), [&h,p]( auto& kv ){
					h = move( kv.second );
					try{
						auto pJson = mu<json>( Json::Parse(move(*p->mutable_result())) );
						h.promise().SetResult( move(pJson) );
					}
					catch( const nlohmann::json::exception& e ){
						h.promise().SetResult( Exception{e.what()} );
					}
					return true;
				} )){
					h.resume();
				}
				else
					ERR( "No returnCall found for request_id='{}'", p->request_id() );
			}
			else if( auto p = item.has_exception() ? item.mutable_exception() : nullptr; p ){
				HCoroutine h{};
				if( _returnCalls.erase_if(p->request_id(), [&h,p]( auto& kv ){
					h = move( kv.second );
					h.promise().SetResult( Exception{p->what()} );
					return true;
				} )){
					h.resume();
				}
				else
					ERR( "({}}No returnCall found.", p->request_id() );
			}			
			else
				ERR( "Unknown message type." );
		}
	}

	up<Proto::RequestString> ToRequestString( Proto::EFields field, uint32 id, sv text ){
		auto pResult = mu<Proto::RequestString>();
		pResult->set_field( field );
		pResult->set_id( id );
		pResult->set_value( text.data(), text.size() );

		return pResult;
	}
	α ServerSink::SendCustom( uint32 requestId, const std::string& bytes )ι->void{
		auto pCustom = mu<Proto::CustomMessage>();
		pCustom->set_requestid( requestId );
		pCustom->set_message( bytes );

		Proto::ToServer t;
		t.add_messages()->set_allocated_custom( pCustom.release() );
		base::Write( move(t) );
	}
/*	α ServerSink::SendAuthenticateComplete( Proto::AuthenticateComplete&& m )ι->void{
		Proto::ToServer t;
		t.add_messages()->set_allocated_authenticate_complete( new Proto::AuthenticateComplete{move(m)} );
		if( _pServerSink )
			_pServerSink->Write( move(t) );
		else{
			auto _logTag = Logging::_logTag;
			WARN( "SendAuthenticateComplete - _pServerSink is null" );
		}
	}
*/
	α ServerSink::Write( const MessageBase& m, TimePoint time, vector<string>* pValues )ι->void{
		auto pProto = mu<Proto::Message>();
		pProto->set_allocated_time( new Proto::Timestamp{IO::Proto::ToTimestamp(time)} );
		pProto->set_level( (Proto::ELogLevel)m.Level );
		pProto->set_messageid( (uint32)m.MessageId );
		pProto->set_fileid( (uint32)m.FileId );
		pProto->set_functionid( (uint32)m.FunctionId );
		pProto->set_linenumber( (uint32)m.LineNumber );
		pProto->set_userid( (uint32)m.UserId );
		pProto->set_threadid( m.ThreadId );
		if( pValues ){
			for( auto& value : *pValues )
				pProto->add_variables( move(value) );
		}
		auto processMessage = [&m, this]( Proto::ToServer& t ){
			if( _messagesSent.emplace(m.MessageId) )
				t.add_messages()->set_allocated_string( ToRequestString(Proto::EFields::MessageId, (uint32)m.MessageId, m.MessageView).release() );
			if( _filesSent.emplace(m.FileId) ){
				t.add_messages()->set_allocated_string( ToRequestString(Proto::EFields::FileId, (uint32)m.FileId, m.File).release() );
				uint crc = Calc32RunTime(m.File);
				ASSERT( crc==m.FileId );
			}
			if( _functionsSent.emplace(m.FunctionId) )
				t.add_messages()->set_allocated_string( ToRequestString(Proto::EFields::FunctionId, (uint32)m.FunctionId, m.Function).release() );
		};

		if( _stringsLoaded ){
			Proto::ToServer t;
			{
				AtomicGuard l{ _bufferMutex };
				if( _buffer.messages_size() ){
					for( auto i=0; i<_buffer.messages_size(); ++i ){
						auto pMessage = _buffer.mutable_messages( i );
						if( pMessage->has_message() )
							t.add_messages()->set_allocated_message( pMessage->release_message() );
						else if( pMessage->has_string() ){
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
			if( Server::_sendStatus.test() ){
				Server::_sendStatus.clear();
				t.add_messages()->set_allocated_status( GetStatus().release() );
			}
			base::Write( move(t) );
		}
		else{
			AtomicGuard l{ _bufferMutex };
			_buffer.add_messages()->set_allocated_message( pProto.release() );;
			processMessage( _buffer );
		}
	}

	atomic<uint> _requestId = 0;
	α LoginAwait( str domain, str loginName, uint32 providerId, HCoroutine h )ι->void{
		var requestId = ++_requestId;
		_returnCalls.emplace( requestId, move(h) );
		if( auto p=_pServerSink; p )
			p->Write( ToServerMsg::AddLogin(domain, loginName, providerId, requestId) );
		else
			Resume( Exception{"No connection to Server"}, move(h) );
	}
	
	AddLoginAwait::AddLoginAwait( str domain, str loginName, uint32 providerId, SL sl )ι:
		AsyncAwait{ [&, provId=providerId](auto h){ 
			LoginAwait(domain, loginName, provId, move(h) ); 
		}, sl, "LoginAwait" }
	{}
	
	α GraphQL( str query, HCoroutine&& h, SL sl )ι{
		var requestId = ++_requestId;
		_returnCalls.emplace( requestId, move(h) );
		if( auto p=_pServerSink; p ){
			TRACESL( "({})GraphQL - query={}", requestId, query );
			p->Write( ToServerMsg::GraphQL(query, requestId) );
		}
		else
			Resume( Exception{"No connection to Server"}, move(h) );
	}

	GraphQLAwait::GraphQLAwait( str query, SL sl )ι:
		AsyncAwait{ [&](auto h){ GraphQL(query, move(h), sl ); }, sl, "GraphQL" }{}

	α SessionInfoAwait::await_suspend( HCoroutine h )ι->void{
		IAwait::await_suspend( h );
		Proto::ToServer t;
		auto p = t.add_messages()->mutable_session_info(); p->set_session_id( _sessionId );
		if( auto p=_pServerSink; p ){
			{
				lg _{ _sessionInfoHandleMutex };
				_sessionInfoHandles[_sessionId].push_back( move(h) );
			}
			p->Write( move(t) );
		}
		else
			Resume( Exception{"No connection to Server"}, move(h) );
	};


	namespace Messages{
		ServerMessage::ServerMessage( const MessageBase& base, vector<string> values )ι:
			Message{ base },
			Timestamp{ Clock::now() },
			Variables{ move(values) }{
			ThreadId = Threading::GetThreadId();
			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}

		ServerMessage::ServerMessage( const ServerMessage& rhs ):
			Message{ rhs },
			Timestamp{ rhs.Timestamp },
			Variables{ rhs.Variables },
			_pFunction{ rhs._pFunction ? mu<string>(*rhs._pFunction) : nullptr }{
			if( _pFunction )
				Function = _pFunction->c_str();
		}
	}
}