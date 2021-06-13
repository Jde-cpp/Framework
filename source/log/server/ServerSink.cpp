#include "ServerSink.h"
//#include "ReceivedMessages.h"
#include "../../threading/Thread.h"
#include "../../DateTime.h"
#define var const auto

namespace Jde::Logging
{
	shared_ptr<IServerSink> _pInstance;
	IServerSink::~IServerSink()
	{
		DBGX( "{}"sv, "~IServerSink~" );
		SetServerSink( nullptr );
		DBGX( "{}"sv, "_pServerSink = nullptr" );
	}
	void IServerSink::Destroy()noexcept
	{
		DBGX( "{}"sv, "IServerSink::Destroy" );
		_pInstance = nullptr;
	}
	sp<IServerSink> IServerSink::GetInstnace()noexcept
	{
		return _pInstance;
	}

	sp<ServerSink> ServerSink::Create( sv host, uint16 port )noexcept(false)
	{
		ASSERT( !_pInstance );
		auto pInstance = sp<ServerSink>( new ServerSink{host, port} );
		_pInstance = pInstance;
		pInstance->UnPause();
		return pInstance;
	}

	ServerSink::ServerSink( sv host, uint16 port )noexcept:
		Interrupt{ "AppServerSend", 1s, true },
		ProtoBase{ host, port, "ServerSocket" },
		_messages{}
	{
		Startup( "AppServerReceive" );
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

	void ServerSink::OnReceive( std::shared_ptr<Proto::FromServer> pFromServer )noexcept
	{
		DBG( "ServerSink::OnReceive - count='{}'"sv, pFromServer->messages_size() );
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
				DBGX( "Acknowledged - instance id={}"sv, ack.instanceid() );
				auto pTransmission = make_shared<Proto::ToServer>();
				auto pInstance = new Proto::Instance();
				_applicationName = IApplication::ApplicationName();//Path().stem().string()=="Jde" ? IApplication::Path().extension().string().substr(1) : IApplication::Path().stem().string();
				pInstance->set_applicationname( _applicationName );
				pInstance->set_hostname( IApplication::HostName() );
				pInstance->set_processid( (int32)IApplication::ProcessId() );
				pInstance->set_starttime( (google::protobuf::uint32)Clock::to_time_t(Logging::StartTime()) );

				pTransmission->add_messages()->set_allocated_instance( pInstance );
				Write( pTransmission );
				_instanceId = ack.instanceid();
			}
			else if( item.has_loglevels() )
			{
				var& levels = item.loglevels();
				Logging::SetLogLevel( (ELogLevel)levels.client(), (ELogLevel)levels.server() );
				INFO_ONCE( "'{}' started at '{}'"sv, _applicationName, ToIsoString(Logging::StartTime()) );
				DBG( "'{}' started at '{}'"sv, _applicationName, ToIsoString(Logging::StartTime()) );
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
					ERR0_ONCE( "No custom function set"sv );
			}
		}
	}


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

	Proto::RequestString* ToRequestString( Proto::EFields field, uint32 id, sv text )
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
			pTransmission->add_messages()->set_allocated_message( pProto );
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
			ThreadId = Threading::GetThreadId();
			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}
		Message::Message( ELogLevel level, sv message, sv file, sv function, uint line, const vector<string>& values )noexcept:
			MessageBase{ level, make_shared<string>(message), file, function, line },
			Timestamp{ Clock::now() },
			Variables{ values }
		{
			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}
		Message::Message( ELogLevel level, sv message, const std::string& file, const std::string& function, uint line, const vector<string>& values )noexcept:
			MessageBase{ level, make_shared<string>(message), file, function, line },
			Timestamp{ Clock::now() },
			Variables{ values },
			_pFile{ make_shared<string>(file) },
			_pFunction{ make_shared<string>(function) }
		{
			File = *_pFile;
			Function = *_pFunction;
			Fields |= EFields::Timestamp | EFields::ThreadId | EFields::Thread;
		}
	}
}