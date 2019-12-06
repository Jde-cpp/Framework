#include "ServerSink.h"
//#include "ReceivedMessages.h"
#include "../../Diagnostics.h"
#include "../../threading/Thread.h"

#define var const auto

namespace Jde::Logging
{
	//using Messages::EMessages;

	shared_ptr<IServerSink> _pInstance;
	IServerSink::~IServerSink()
	{ 
		DBGX( "{}", "~IServerSink~" );
		SetServerSink( nullptr );
		DBGX( "{}", "_pServerSink = nullptr" );
	}
	void IServerSink::Destroy()noexcept
	{
		DBGX( "{}", "IServerSink::Destroy" );
		_pInstance = nullptr;
	}
	sp<IServerSink> IServerSink::GetInstnace()noexcept
	{
		return _pInstance;
	}
	

	sp<ServerSink> ServerSink::Create( string_view host, uint16_t port )noexcept(false)
	{
		ASSERT( !_pInstance );
		auto pInstance = sp<ServerSink>( new ServerSink{host, port} );
		_pInstance = pInstance;
		pInstance->UnPause();
		return pInstance;
	}

	ServerSink::ServerSink( string_view host, uint16 port )noexcept:
		Interrupt{ "ServerSink", 1s, true },
		ProtoBase{ host, port, "ServerSocket" }
	{
		Startup();
	}

	void ServerSink::Destroy()noexcept
	{
		// Disconnect();
		// while( _connected )
		// 	std::this_thread::yield();
		IServerSink::Destroy();
	}
	void ServerSink::OnConnected()noexcept
	{
		_connected = true;
	}


	void ServerSink::OnDisconnect()noexcept
	{
		DBG0( "Disconnected from LogServer." );
		_messagesSent.clear();
		_filesSent.clear();
		_functionsSent.clear();
		_usersSent.clear();
		_threadsSent.clear();
		_instanceId = 0;
		_lastConnectionCheck = Clock::now();
		_connected = false;
	}

	void ServerSink::OnReceive( std::shared_ptr<Proto::FromServer> pFromServer )noexcept
	{
		DBG( "ServerSink::OnReceive - count='{}'", pFromServer->messages_size() );
		for( var& item : pFromServer->messages() )
		{
			if( item.has_strings() )
			{
				var& strings = item.strings();
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
				DBGX( "Acknowledged - instance id={}", ack.instanceid() );
				auto pTransmission = make_shared<Proto::ToServer>();
				auto pInstance = new Proto::Instance();
				var applicationName = Diagnostics::ApplicationName().stem().string()=="Jde" ? Diagnostics::ApplicationName().extension().string().substr(1) : Diagnostics::ApplicationName().stem().string();
				pInstance->set_applicationname( applicationName );
				pInstance->set_hostname( Diagnostics::HostName() );
				pInstance->set_processid( (int32)Diagnostics::ProcessId() );
				pInstance->set_starttime( (google::protobuf::uint32)Clock::to_time_t(Logging::StartTime()) );

				pTransmission->add_messages()->set_allocated_instance( pInstance );
				Write( pTransmission );
				_instanceId = ack.instanceid();
			}
			// else if( item.has_generic() )
			// {
			// 	var generic = item.generic();
			// 	if( generic.what()==Proto::EFromServer::Shutdown )
			// }
			else if( item.has_loglevels() )
			{
				var& levels = item.loglevels();
				Logging::SetLogLevel( (ELogLevel)levels.client(), (ELogLevel)levels.server() );
			}
			else if( item.has_custom() )
			{
				if( _customFunction )
				{
					var custom = item.custom();
					var requestId = custom.requestid();
					var bytes = make_shared<string>( custom.message() );
					std::thread( [&,requestId,bytes](){_customFunction(requestId, bytes);} ).detach(); //
				}
				else
					ERR0_ONCE( "No custom function set" );
			}
		}
	}
	

/*	void ServerSink::OnIncoming( IO::IncomingMessage& / *returnMessage* / )
	{
		//var messageType = (EMessages)returnMessage.ReadInt32();
// 		switch( messageType )
// 		{
// 		case EMessages::None:
// 			break;
// 		case EMessages::Acknowledgement:
// //			HandleAcknowledgement();
// 			break;
// 		case EMessages::Application:
// //			HandleAcknowledgement();
// 			break;
// 		case EMessages::Message:
// //			HandleAcknowledgement();
// 			break;
// 		case EMessages::Strings:
// //			HandleAcknowledgement();
// 			break;
// 		}
	}*/
	void ServerSink::Log( const MessageBase& message )noexcept
	{
		_messages.Push( Messages::Message(message) );
	}
	void ServerSink::Log( const Messages::Message& message )noexcept
	{
		_messages.Push( message );
	}

	void ServerSink::Log( const MessageBase& message, const vector<string>& values )noexcept
	{
		_messages.Push( Messages::Message(message, values) );
	}

	Proto::RequestString* ToRequestString( Proto::EFields field, uint32 id, string_view text )
	{
		auto pResult = new Proto::RequestString();
		pResult->set_field( field );
		pResult->set_id( id );
		pResult->set_value( text.data(), text.size() );

		return pResult;
	}
	void ServerSink::SendCustom( uint32 requestId, const std::string& bytes )noexcept
	{
		auto pCustom = new Proto::CustomMessage();
		pCustom->set_requestid( requestId );
		pCustom->set_message( bytes );

		auto pTransmission = make_shared<Proto::ToServer>();
		pTransmission->add_messages()->set_allocated_custom( pCustom );
		Write( pTransmission );
	}
	void ServerSink::OnTimeout()/*noexcept*/
	{
		if( !_connected )
		{
			if( Clock::now()>_lastConnectionCheck+10s )
			{
				Connect();
				_lastConnectionCheck = Clock::now();
			}
			else
				return;
		}
		if( (!_messages.size() && !SendStatus) || !_stringsLoaded )
			return;
		auto pTransmission = make_shared<Proto::ToServer>();
		std::function<void(Messages::Message&)> func = [&](const Messages::Message& message)
		{
			auto pProto = new Proto::Message();
			var seconds = Clock::to_time_t( message.Timestamp );
			var nanos = std::chrono::duration_cast<std::chrono::nanoseconds>( message.Timestamp-TimePoint{} )-std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::seconds(seconds) );
			auto pTime = new google::protobuf::Timestamp();
			pTime->set_seconds( seconds );
			pTime->set_nanos( (int)nanos.count() );
			pProto->set_allocated_time( pTime );

			pProto->set_level( (Proto::ELogLevel)message.Level );
			pProto->set_messageid( (uint32)message.MessageId );
			if( _messagesSent.emplace(message.MessageId) )
				pTransmission->add_messages()->set_allocated_string( ToRequestString(Proto::EFields::MessageId, (uint32)message.MessageId, message.MessageView) );

			pProto->set_fileid( (uint32)message.FileId );
			if( _filesSent.emplace(message.FileId) )
				pTransmission->add_messages()->set_allocated_string( ToRequestString(Proto::EFields::FileId, (uint32)message.FileId, message.File) );

			pProto->set_functionid( (uint32)message.FunctionId );
			if( _functionsSent.emplace(message.FunctionId) )
				pTransmission->add_messages()->set_allocated_string( ToRequestString(Proto::EFields::FunctionId, (uint32)message.FunctionId, message.Function) );
			pProto->set_linenumber( (uint32)message.LineNumber );
			pProto->set_userid( (uint32)message.UserId );
			pProto->set_threadid( message.ThreadId );
			for( var& variable : message.Variables )
				pProto->add_variables( variable );
			//DBGX("size={}", pTransmission->messages_size() );
			pTransmission->add_messages()->set_allocated_message( pProto );
			//DBGX("size={}", pTransmission->messages_size() );
		};
		_messages.ForEach( func );
		if( SendStatus )
		{
			pTransmission->add_messages()->set_allocated_status( GetAllocatedStatus() );
			SendStatus = false;
		}
		if( _connected )
			Write( pTransmission );
	}

	namespace Messages
	{
		Message::Message(const MessageBase& base, const vector<string>& values)noexcept:
			MessageBase{ base },
			Timestamp{ Clock::now() },
			Variables{ values }
		{
			//Thread = Threading::GetThreadDescription();
			ThreadId = Threading::ThreadId;

			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}
		Message::Message( ELogLevel level, std::string_view message, std::string_view file, std::string_view function, uint line, const vector<string>& values )noexcept:
			MessageBase{ level, message, file, function, line },
			Timestamp{ Clock::now() },
			Variables{ values },
			_pMessage{ make_shared<string>(message) }
		{
			MessageView = *_pMessage;
			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}

		Message::Message( const Message& other )noexcept:
			MessageBase{ other },
			Timestamp{ Clock::now() },
			Variables{ other.Variables },
			_pMessage{ other._pMessage }
		{
			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}
	}
}