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

#define 🚪 Γ auto

namespace Jde::Logging
{
	🚪 Server()noexcept->up<Logging::IServerSink>&; 🚪 SetServer( up<Logging::IServerSink> p )noexcept->void;
	🚪 ServerLevel()noexcept->ELogLevel; 🚪 SetServerLevel( ELogLevel serverLevel )noexcept->void;

	struct Γ IServerSink : private boost::noncopyable
	{
		IServerSink()=default;
		virtual ~IServerSink();

		virtual void Log( Messages::Message& message )noexcept=0;
		virtual void Log( const MessageBase& messageBase )noexcept=0;
		virtual void Log( const MessageBase& messageBase, vector<string>& values )noexcept=0;

		bool ShouldSendMessage( uint messageId )noexcept{ return _messagesSent.emplace(messageId); }
		bool ShouldSendFile( uint messageId )noexcept{ return _filesSent.emplace(messageId); }
		bool ShouldSendFunction( uint messageId )noexcept{ return _functionsSent.emplace(messageId); }
		bool ShouldSendUser( uint messageId )noexcept{ return _usersSent.emplace(messageId); }
		bool ShouldSendThread( uint messageId )noexcept{ return _threadsSent.emplace(messageId); }
		virtual void SendCustom( uint32 /*requestId*/, str /*bytes*/ )noexcept{ CRITICAL("SendCustom not implemented"sv); }
		static bool Enabled()noexcept{ return _enabled; }
		atomic<bool> SendStatus{false};
	protected:
		UnorderedSet<uint> _messagesSent;
		UnorderedSet<uint> _filesSent;
		UnorderedSet<uint> _functionsSent;
		UnorderedSet<uint> _usersSent;
		UnorderedSet<uint> _threadsSent;
	private:
		static bool _enabled;
	};
	namespace Messages
	{
		struct Γ Message final : Message2
		{
			Message( const MessageBase& base )noexcept:
				Message2{ base }
			{}
			Message( const Message& base );
			Message( const MessageBase& base, vector<string> values )noexcept;
			Message( const Message2& b, vector<string> values )noexcept:
				Message2{ b },
				Variables{ move(values) }
			{}
			const TimePoint Timestamp{ Clock::now() };
			vector<string> Variables;
		private:
			unique_ptr<string> _pFunction;
		};
	}

	typedef IO::Sockets::TProtoClient<Logging::Proto::ToServer,Logging::Proto::FromServer> ProtoBase;
	struct ServerSink final: IServerSink, ProtoBase
	{
		using base=ProtoBase;
		static α Create()noexcept->ServerSink*;
		ServerSink()noexcept(false);
		~ServerSink(){DBGX("{}"sv, "~ServerSink");}
		α Log( Messages::Message& m )noexcept->void override{ Write( m, m.Timestamp, &m.Variables ); }
		α Log( const MessageBase& m )noexcept->void override{ Write( m, Clock::now() ); }
		α Log( const MessageBase& m, vector<string>& values )noexcept->void override{ Write( m, Clock::now(), &values ); };

		α OnDisconnect()noexcept->void override;
		α OnConnected()noexcept->void override;
		α SendCustom( uint32 requestId, str bytes )noexcept->void override;
		α SetCustomFunction( function<Coroutine::Task2(uint32,string&&)>&& fnctn )noexcept{_customFunction=fnctn;}
	private:
		α Write( const MessageBase& message, TimePoint time, vector<string>* pValues=nullptr )noexcept->void;
		α OnReceive( Logging::Proto::FromServer& fromServer )noexcept->void override;
		uint _instanceId{0};
		atomic<bool> _stringsLoaded{false};
		string _applicationName;
		function<Coroutine::Task2(uint32,string&&)> _customFunction;
		Proto::ToServer _buffer; atomic<bool> _bufferMutex;
	};
}
#undef 🚪