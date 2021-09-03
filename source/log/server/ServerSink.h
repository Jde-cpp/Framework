#pragma once
#include "../../threading/Interrupt.h"
#include "../../collections/Queue.h"
#include "../../collections/UnorderedSet.h"
#include "../../io/sockets/ProtoClient.h"
#include <jde/coroutine/Task.h>
#pragma warning(push)
#pragma warning( disable : 4127 )
#pragma warning( disable : 5054 )
	#include "./proto/messages.pb.h"
#pragma warning(pop)

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
		//IServerSink( ELogLevel serverLevel )noexcept:_level{serverLevel}{}
		virtual ~IServerSink();

		virtual void Destroy()noexcept;

		virtual void Log( Messages::Message&& message )noexcept=0;
		virtual void Log( MessageBase&& messageBase )noexcept=0;
		virtual void Log( MessageBase messageBase, vector<string> values )noexcept=0;

		//ELogLevel GetLogLevel()const noexcept; void SetLogLevel(ELogLevel level)noexcept;

		bool ShouldSendMessage( uint messageId )noexcept{ return _messagesSent.emplace(messageId); }
		bool ShouldSendFile( uint messageId )noexcept{ return _filesSent.emplace(messageId); }
		bool ShouldSendFunction( uint messageId )noexcept{ return _functionsSent.emplace(messageId); }
		bool ShouldSendUser( uint messageId )noexcept{ return _usersSent.emplace(messageId); }
		bool ShouldSendThread( uint messageId )noexcept{ return _threadsSent.emplace(messageId); }
		virtual void SendCustom( uint32 /*requestId*/, const std::string& /*bytes*/ )noexcept{ CRITICAL("SendCustom not implemented"sv); }
		std::atomic<bool> SendStatus{false};
	protected:
		sp<IServerSink> GetInstnace()noexcept;
		UnorderedSet<uint> _messagesSent;
		UnorderedSet<uint> _filesSent;
		UnorderedSet<uint> _functionsSent;
		UnorderedSet<uint> _usersSent;
		UnorderedSet<uint> _threadsSent;
	private:
		//ELogLevel _level{ELogLevel::Error};
	};
	namespace Messages
	{
		struct JDE_NATIVE_VISIBILITY Message final : Message2
		{
			// Message( const Message2& base )noexcept:
			// 	Message2{ base }
			// {}
			Message( const MessageBase& base )noexcept:
				Message2{ base }
			{}
			Message( const Message& base );
			Message( const MessageBase& base, vector<string> values )noexcept;
			//Message( MessageBase&& base, vector<string>&& values )noexcept;
			Message( const Message2& b, vector<string> values )noexcept:
				Message2{ b },
				Variables{ move(values) }
			{}
			//Message( ELogLevel level, sv message, sv file, sv function, int line, vector<string>&& values )noexcept;
			//Message( ELogLevel level, string&& message, string&& file, string&& function, int line, vector<string>&& values )noexcept;

			const TimePoint Timestamp{ Clock::now() };
			vector<string> Variables;
		private:
			//unique_ptr<string> _pFile;
			unique_ptr<string> _pFunction;
		};
	}
	typedef IO::Sockets::TProtoClient<Logging::Proto::ToServer,Logging::Proto::FromServer> ProtoBase;
	struct ServerSink final: IServerSink, Threading::Interrupt, ProtoBase
	{
		ServerSink()noexcept(false);
		//static JDE_NATIVE_VISIBILITY sp<ServerSink> Create( sv host, uint16 port )noexcept(false);
		~ServerSink(){DBGX("{}"sv, "~ServerSink");}
		void Log( Messages::Message&& message )noexcept override;
		void Log( MessageBase&& messageBase )noexcept override;
		void Log( MessageBase messageBase, vector<string> values )noexcept override;

		void OnTimeout()noexcept override;
		void OnAwake()noexcept override{OnTimeout();}//not expecting...
		void OnDisconnect()noexcept override;
		void OnConnected()noexcept override;
		void Destroy()noexcept override;
		void SendCustom( uint32 requestId, const std::string& bytes )noexcept override;
		void SetCustomFunction( function<Coroutine::Task2(uint32,string&&)>&& fnctn )noexcept{_customFunction=fnctn;}
	private:
		void OnReceive( Logging::Proto::FromServer& fromServer )noexcept override;
		TimePoint _lastConnectionCheck;
		QueueMove<Messages::Message> _messages;
		uint _instanceId{0};
		std::atomic<bool> _stringsLoaded{false};
		string _applicationName;
		function<Coroutine::Task2(uint32,string&&)> _customFunction;
	};
}}