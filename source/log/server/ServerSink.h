#pragma once
#include "../../threading/Interrupt.h"
#include "../../collections/Queue.h"
#include "../../collections/UnorderedSet.h"
#include "../../io/sockets/ProtoClient.h"
#include "./proto/messages.pb.h"
//#include "ReceivedMessages.h"

namespace Jde
{
	namespace IO
	{
		class IncomingMessage;
		namespace Sockets{ class Client; }
	}
namespace Logging
{
	struct JDE_NATIVE_VISIBILITY IServerSink
	{
		IServerSink()=default;
		IServerSink( ELogLevel serverLevel )noexcept:_level{serverLevel}{}
		virtual ~IServerSink();

		virtual void Destroy()noexcept;

		//virtual void Log( const MessageBase& messageBase )noexcept=0;
		//virtual void Log( const MessageBase& messageBase, const vector<string>& values )noexcept=0;
		virtual void Log( const Messages::Message& message )noexcept=0;
		virtual void Log( const MessageBase& messageBase )noexcept=0;
		virtual void Log( const MessageBase& messageBase, const vector<string>& values )noexcept=0;

		ELogLevel GetLogLevel()const noexcept{ return _level; } void SetLogLevel(ELogLevel level)noexcept{ _level=level; }

		bool ShouldSendMessage( uint messageId )noexcept{ return _messagesSent.emplace(messageId); }
		bool ShouldSendFile( uint messageId )noexcept{ return _filesSent.emplace(messageId); }
		bool ShouldSendFunction( uint messageId )noexcept{ return _functionsSent.emplace(messageId); }
		bool ShouldSendUser( uint messageId )noexcept{ return _usersSent.emplace(messageId); }
		bool ShouldSendThread( uint messageId )noexcept{ return _threadsSent.emplace(messageId); }
		virtual void SendCustom( uint32 /*requestId*/, const std::string& /*bytes*/ )noexcept{ CRITICAL0("SendCustom not implemented"sv); }
		std::atomic<bool> SendStatus{false};
	protected:
		sp<IServerSink> GetInstnace()noexcept;
		Collections::UnorderedSet<uint> _messagesSent;
		Collections::UnorderedSet<uint> _filesSent;
		Collections::UnorderedSet<uint> _functionsSent;
		Collections::UnorderedSet<uint> _usersSent;
		Collections::UnorderedSet<uint> _threadsSent;
	private:
		ELogLevel _level{ELogLevel::Error};
	};
	namespace Messages
	{
		struct JDE_NATIVE_VISIBILITY Message : public MessageBase
		{
			Message( const MessageBase& base ) ://get application id...
				MessageBase(base)
			{
				//MessageView = *_pMessage;
			}
			Message(const MessageBase& base, const vector<string>& values )noexcept;
			//Message( const Message& other )noexcept;
			Message( ELogLevel level, std::string_view message, std::string_view file, std::string_view function, uint line, const vector<string>& values )noexcept;
			Message( ELogLevel level, std::string_view message, const std::string& file, const std::string& function, uint line, const vector<string>& values )noexcept;
			//Message( IO::IncomingMessage& message, EFields fields )noexcept(false);

			TimePoint Timestamp{ Clock::now() };
			//string Thread;
			//string User;
			vector<string> Variables;
			//void SetSessionId( uint value )noexcept{ if( value ) Fields|=EFields::SessionId; _sessionId = value; }
			//std::ostream& to_stream( std::ostream& os, ServerSink* pSink=nullptr )const noexcept;
		private:
			//const sp<string> _pMessage;
			const sp<string> _pFile;
			const sp<string> _pFunction;
			//uint _sessionId{0};
		};
	}
	typedef IO::Sockets::TProtoClient<Logging::Proto::ToServer,Logging::Proto::FromServer> ProtoBase;
	struct ServerSink : public IServerSink, Threading::Interrupt, public ProtoBase
	{
		static JDE_NATIVE_VISIBILITY sp<ServerSink> Create( string_view host, uint16_t port )noexcept(false);
		~ServerSink(){DBGX("{}"sv, "~ServerSink");}
		//void Log( ELogLevel level, std::string_view message, uint messageId, std::string_view file, std::string_view function, uint line );
		void Log( const Messages::Message& message )noexcept override;
		void Log( const MessageBase& messageBase )noexcept override;
		void Log( const MessageBase& messageBase, const vector<string>& values )noexcept override;

		void OnTimeout()/*noexcept*/ override;
		void OnAwake()noexcept override{OnTimeout();}//not expecting...
		//static void OnIncoming( IO::IncomingMessage& returnMessage );
		void OnDisconnect()noexcept override;
		void OnConnected()noexcept override;
		void Destroy()noexcept override;
		void SendCustom( uint32 requestId, const std::string& bytes )noexcept override;
		void SetCustomFunction( function<void(uint32,sp<string>)>&& fnctn )noexcept{_customFunction=fnctn;}
	private:
		ServerSink( string_view host, uint16 port )noexcept;
		void OnReceive( std::shared_ptr<Logging::Proto::FromServer> pFromServer )noexcept;
		TimePoint _lastConnectionCheck;
		QueueValue<Messages::Message> _messages;
		uint _instanceId{0};
		std::atomic<bool> _stringsLoaded{false};
		string _applicationName;
		function<void(uint32,sp<string>)> _customFunction;
	};
}}