#include "ServerSink.h"
#include "../../threading/Thread.h"
#include "../../DateTime.h"
#define var const auto

namespace Jde::Logging
{
	ELogLevel _serverLogLevel{ ELogLevel::None };
	up<Logging::IServerSink> _pServerSink;
	ELogLevel _sinkLogLevel = Settings::TryGet<ELogLevel>( "logging/server/diagnosticsLevel" ).value_or( ELogLevel::Trace );
}

namespace Jde
{
	α Logging::Server()noexcept->up<Logging::IServerSink>&{ return _pServerSink; }
	α Logging::SetServer( up<Logging::IServerSink> p )noexcept->void{ _pServerSink=move(p); if( _pServerSink ) _serverLogLevel=Settings::TryGet<ELogLevel>("logging/server/level").value_or(ELogLevel::Error); }
	α Logging::ServerLevel()noexcept->ELogLevel{ return _serverLogLevel; }
	α Logging::SetServerLevel( ELogLevel serverLevel )noexcept->void{ _serverLogLevel=serverLevel; }
}

namespace Jde::Logging
{
	bool IServerSink::_enabled{ Settings::TryGet<PortType>("logging/server/port").value_or(0)!=0 };
	IServerSink::~IServerSink()
	{
		DBGX( "{}"sv, "~IServerSink" );
		SetServer( nullptr );
		DBGX( "{}"sv, "_pServerSink = nullptr" );
	}

	α ServerSink::Create()noexcept->ServerSink*
	{
		try
		{
			SetServer( make_unique<ServerSink>() );
		}
		catch( const IException& )
		{}
		return (ServerSink*)_pServerSink.get();
	}

	ServerSink::ServerSink()noexcept(false):
		ProtoBase{ "logging/server", 0 }//,don't wan't default port
	{
		INFO( "ServerSink::ServerSink( path='{}', Host='{}', Port='{}' )", "logging/server", Host, Port );
	}

	α ServerSink::OnConnected()noexcept->void{ _connected = true; }


	α ServerSink::OnDisconnect()noexcept->void
	{
		DBG( "Disconnected from LogServer."sv );
		_messagesSent.clear();
		_filesSent.clear();
		_functionsSent.clear();
		_usersSent.clear();
		_threadsSent.clear();
		_instanceId = 0;
		_connected = false;
	}

	α ServerSink::OnReceive( Proto::FromServer& transmission )noexcept->void
	{
		for( auto i2=0; i2<transmission.messages_size(); ++i2 )
		{
			auto& item = *transmission.mutable_messages( i2 );
			if( item.has_strings() )
			{
				var& strings = item.strings();
				LOG( _sinkLogLevel, "received '{}' strings", strings.messages_size()+strings.files_size()+strings.functions_size()+strings.threads_size() );
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
				DBGX( "Acknowledged - instance id={}"sv, ack.instanceid() );
				Proto::ToServer t;
				auto pInstance = make_unique<Proto::Instance>();
				_applicationName = IApplication::ApplicationName();
				pInstance->set_applicationname( _applicationName );
				pInstance->set_hostname( IApplication::HostName() );
				pInstance->set_processid( (int32)OSApp::ProcessId() );
				pInstance->set_starttime( (google::protobuf::uint32)Clock::to_time_t(Logging::StartTime()) );

				t.add_messages()->set_allocated_instance( pInstance.release() );
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
		}
	}

	up<Proto::RequestString> ToRequestString( Proto::EFields field, uint32 id, sv text )
	{
		auto pResult = make_unique<Proto::RequestString>();
		pResult->set_field( field );
		pResult->set_id( id );
		pResult->set_value( text.data(), text.size() );

		return pResult;
	}
	α ServerSink::SendCustom( uint32 requestId, const std::string& bytes )noexcept->void
	{
		auto pCustom = make_unique<Proto::CustomMessage>();
		pCustom->set_requestid( requestId );
		pCustom->set_message( bytes );

		Proto::ToServer t;
		t.add_messages()->set_allocated_custom( pCustom.release() );
		base::Write( t );
	}

	α ServerSink::Write( const MessageBase& m, TimePoint time, vector<string>* pValues )noexcept->void
	{
		auto pProto = make_unique<Proto::Message>();
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
				Threading::AtomicGuard l{ _bufferMutex };
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
							UnorderedSet<uint>& set = pString->field()==Proto::EFields::MessageId ? _messagesSent : pString->field()==Proto::EFields::FileId ? _filesSent : _functionsSent;
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
			Threading::AtomicGuard l{ _bufferMutex };
			_buffer.add_messages()->set_allocated_message( pProto.release() );;
			processMessage( _buffer );
		}
	}
	namespace Messages
	{
		ServerMessage::ServerMessage( const MessageBase& base, vector<string> values )noexcept:
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
			//_pFile{ rhs._pFile ? make_unique<string>(*rhs._pFile) : nullptr },
			_pFunction{ rhs._pFunction ? make_unique<string>(*rhs._pFunction) : nullptr }
		{
			//if( _pFile )
			//	File = *_pFile;
			if( _pFunction )
				Function = *_pFunction;
		}
	}
}