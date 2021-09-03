#include "ServerSink.h"
#include "../../threading/Thread.h"
#include "../../DateTime.h"
#define var const auto

namespace Jde::Logging
{
	ELogLevel _sinkLogLevel = Settings::TryGet<ELogLevel>( "logging/server/diagnosticsLevel" ).value_or( ELogLevel::Trace );
	IServerSink::~IServerSink()
	{
		DBGX( "{}"sv, "~IServerSink" );
		SetServerSink( nullptr );
		DBGX( "{}"sv, "_pServerSink = nullptr" );
	}
	void IServerSink::Destroy()noexcept
	{
		DBGX( "{}"sv, "IServerSink::Destroy" );
		SetServerSink( nullptr );
		//_pInstance = nullptr;
	}
/*	sp<IServerSink> IServerSink::GetInstnace()noexcept
	{
		return _pInstance;
	}
*/
	ServerSink::ServerSink()noexcept(false):
		Interrupt{ "AppServerSend", 1s, true },
		ProtoBase{ "LogClient", "logging/server", 0 },//don't wan't default port
		_messages{}
	{
	//	Startup( "AppServerReceive" );
	}

	void ServerSink::Destroy()noexcept
	{
		std::this_thread::sleep_for( 1s );//flush messages.
		IServerSink::Destroy();
	}
	void ServerSink::OnConnected()noexcept
	{
		_connected = true;
	}


	void ServerSink::OnDisconnect()noexcept
	{
		DBG( "Disconnected from LogServer."sv );
		_messagesSent.clear();
		_filesSent.clear();
		_functionsSent.clear();
		_usersSent.clear();
		_threadsSent.clear();
		_instanceId = 0;
		_lastConnectionCheck = Clock::now();
		_connected = false;
	}

	void ServerSink::OnReceive( Proto::FromServer& transmission )noexcept
	{
		//DBG( "ServerSink::OnReceive - count='{}'"sv, transmission.messages_size() );
		for( uint i=0; i<transmission.messages_size(); ++i )
		{
			auto& item = *transmission.mutable_messages( i );
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
				Proto::ToServer transmission;
				auto pInstance = make_unique<Proto::Instance>();
				_applicationName = IApplication::ApplicationName();
				pInstance->set_applicationname( _applicationName );
				pInstance->set_hostname( IApplication::HostName() );
				pInstance->set_processid( (int32)OSApp::ProcessId() );
				pInstance->set_starttime( (google::protobuf::uint32)Clock::to_time_t(Logging::StartTime()) );

				transmission.add_messages()->set_allocated_instance( pInstance.release() );
				Write( move(transmission) );
				_instanceId = ack.instanceid();
			}
			else if( item.has_loglevels() )
			{
				var& levels = item.loglevels();
				Logging::SetLogLevel( (ELogLevel)levels.client(), (ELogLevel)levels.server() );
				INFO_ONCE( "'{}' started at '{}'"sv, _applicationName, ToIsoString(Logging::StartTime()) );
				//DBG( "'{}' started at '{}'"sv, _applicationName, ToIsoString(Logging::StartTime()) );
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


	void ServerSink::Log( MessageBase&& message )noexcept
	{
		_messages.Push( Messages::Message(move(message)) );
	}
	void ServerSink::Log( Messages::Message&& message )noexcept
	{
		_messages.Push( move(message) );
	}

	void ServerSink::Log( MessageBase message, vector<string> values )noexcept
	{
		_messages.Push( Messages::Message(move(message), move(values)) );
	}

	up<Proto::RequestString> ToRequestString( Proto::EFields field, uint32 id, sv text )
	{
		auto pResult = make_unique<Proto::RequestString>();
		pResult->set_field( field );
		pResult->set_id( id );
		pResult->set_value( text.data(), text.size() );

		return pResult;
	}
	void ServerSink::SendCustom( uint32 requestId, const std::string& bytes )noexcept
	{
		auto pCustom = make_unique<Proto::CustomMessage>();
		pCustom->set_requestid( requestId );
		pCustom->set_message( bytes );

		Proto::ToServer t;
		t.add_messages()->set_allocated_custom( pCustom.release() );
		Write( t );
	}
	void ServerSink::OnTimeout()noexcept
	{
		bool haveWork = (!_messages.size() && !SendStatus) || !_stringsLoaded;
		if( !haveWork || (!_connected && (Clock::now()<_lastConnectionCheck+10s || !Try([this]{ _lastConnectionCheck = Clock::now(); Connect(); }))) )
			return;

		Proto::ToServer t;
		auto messages = _messages.PopAll();
		for( auto&& message : messages )
		{
			auto pProto = make_unique<Proto::Message>();
			pProto->set_allocated_time( IO::Proto::ToTimestamp(message.Timestamp).release() );

			pProto->set_level( (Proto::ELogLevel)message.Level );
			pProto->set_messageid( (uint32)message.MessageId );
			if( _messagesSent.emplace(message.MessageId) )
				t.add_messages()->set_allocated_string( ToRequestString(Proto::EFields::MessageId, (uint32)message.MessageId, move(message.MessageView)).release() );

			pProto->set_fileid( (uint32)message.FileId );
			if( _filesSent.emplace(message.FileId) )
				t.add_messages()->set_allocated_string( ToRequestString(Proto::EFields::FileId, (uint32)message.FileId, message.File).release() );

			pProto->set_functionid( (uint32)message.FunctionId );
			if( _functionsSent.emplace(message.FunctionId) )
				t.add_messages()->set_allocated_string( ToRequestString(Proto::EFields::FunctionId, (uint32)message.FunctionId, message.Function).release() );
			pProto->set_linenumber( (uint32)message.LineNumber );
			pProto->set_userid( (uint32)message.UserId );
			pProto->set_threadid( message.ThreadId );
			for( var& variable : message.Variables )
				pProto->add_variables( variable );
			t.add_messages()->set_allocated_message( pProto.release() );
		};
		if( SendStatus )
		{
			t.add_messages()->set_allocated_status( GetStatus().release() );
			SendStatus = false;
		}
		Write( t );
	}

	namespace Messages
	{
		Message::Message( const MessageBase& base, vector<string> values )noexcept:
			Message2{ base },
			Timestamp{ Clock::now() },
			Variables{ move(values) }
		{
			ThreadId = Threading::GetThreadId();
			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}


/*		Message::Message( MessageBase&& base, vector<string>&& values )noexcept:
			Message2{ move(base) },
			Timestamp{ Clock::now() },
			Variables{ move(values) }
		{
			std::cout << base.GetType()  << endl;
			ThreadId = Threading::GetThreadId();
			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}
*/
		Message::Message( const Message& rhs ):
			Message2{ rhs },
			Timestamp{ rhs.Timestamp },
			Variables{ rhs.Variables },
			//_pFile{ rhs._pFile ? make_unique<string>(*rhs._pFile) : nullptr },
			_pFunction{ rhs._pFunction ? make_unique<string>(*rhs._pFunction) : nullptr }
		{
			//ASSERT( _pFile && _pFunction );
			//if( _pFile )
			//	File = *_pFile;
			if( _pFunction )
				Function = *_pFunction;
		}
/*		Message::Message( ELogLevel level, sv message, sv file, sv function, int line, vector<string>&& values )noexcept:
			Message2{ message, level, file, function, line },
			Timestamp{ Clock::now() },
			Variables{ move(values) }
		{
			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}
		Message::Message( ELogLevel level, string&& message, string&& file, string&& function, int line, vector<string>&& values )noexcept:
			Message2{ level, move(message), file, function, line },
			Timestamp{ Clock::now() },
			Variables{ move(values) },
			_pFile{ make_unique<string>(move(file)) },
			_pFunction{ make_unique<string>(move(function)) }
		{
			File = *_pFile;
			Function = *_pFunction;
			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}*/
	}
}