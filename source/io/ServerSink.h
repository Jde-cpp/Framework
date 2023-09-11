#pragma once
#include <jde/coroutine/Task.h>
#include "../threading/Interrupt.h"
#include "../collections/Queue.h"
#include "../collections/UnorderedSet.h"
#include "ProtoClient.h"

DISABLE_WARNINGS
#include "./proto/messages.pb.h"
ENABLE_WARNINGS

namespace Jde::Logging
{
	struct IServerSink;
	namespace Messages{ struct ServerMessage; }
	Γ α Server()ι->up<Logging::IServerSink>&; Γ α SetServer( up<Logging::IServerSink> p )ι->void;
	Γ α ServerLevel()ι->ELogLevel; Γ α SetServerLevel( ELogLevel serverLevel )ι->void;

	struct SessionInfoAwait final : IAwait
	{
		SessionInfoAwait( SessionPK sessionId, SRCE )ι:IAwait{sl},_sessionId{sessionId}{}
		α await_suspend( HCoroutine h )ι->void override;
	private:
		SessionPK _sessionId;
	};

	struct Γ IServerSink //: private boost::noncopyable debugging issues
	{
		using ID=uint32;
		IServerSink()ι=default;
		IServerSink( const unordered_set<ID>& msgs )ι:_messagesSent{msgs}{}
		virtual ~IServerSink();

		β IsLocal()ι->bool{ return false; }
		β Log( Messages::ServerMessage& message )ι->void=0;
		β Log( const MessageBase& messageBase )ι->void=0;
		β Log( const MessageBase& messageBase, vector<string>& values )ι->void=0;
		β FetchSessionInfo( SessionPK sessionId )ι->SessionInfoAwait=0;
		α ShouldSendMessage( ID messageId )ι->bool{ return _messagesSent.emplace(messageId); }
		α ShouldSendFile( ID messageId )ι->bool{ return _filesSent.emplace(messageId); }
		α ShouldSendFunction( ID messageId )ι->bool{ return _functionsSent.emplace(messageId); }
		α ShouldSendUser( ID messageId )ι->bool{ return _usersSent.emplace(messageId); }
		α ShouldSendThread( ID messageId )ι->bool{ return _threadsSent.emplace(messageId); }
		β SendCustom( ID /*requestId*/, str /*bytes*/ )ι->void{ CRITICAL("SendCustom not implemented"); }
		Ω Enabled()ι->bool{ return _enabled; }
		atomic<bool> SendStatus{false};
	protected:
		UnorderedSet<ID> _messagesSent;
		UnorderedSet<ID> _filesSent;
		UnorderedSet<ID> _functionsSent;
		UnorderedSet<ID> _usersSent;
		UnorderedSet<ID> _threadsSent;
	private:
		static bool _enabled;
	};
	namespace Messages
	{
		struct Γ ServerMessage final : Logging::Message
		{
			ServerMessage( const MessageBase& base )ι:
				Logging::Message{ base }
			{}
			ServerMessage( const ServerMessage& base );
			ServerMessage( const MessageBase& base, vector<string> values )ι;
			ServerMessage( const Message& b, vector<string> values )ι:
				Message{ b },
				Variables{ move(values) }
			{}
			const TimePoint Timestamp{ Clock::now() };
			vector<string> Variables;
		private:
			up<string> _pFunction;
		};
	}

	typedef IO::Sockets::TProtoClient<Logging::Proto::ToServer,Logging::Proto::FromServer> ProtoBase;
	struct ServerSink final: IServerSink, ProtoBase
	{
		using base=ProtoBase;
		static α Create()ι->ServerSink*;
		ServerSink()ι(false);
		~ServerSink();
		α Log( Messages::ServerMessage& m )ι->void override{ Write( m, m.Timestamp, &m.Variables ); }
		α Log( const MessageBase& m )ι->void override{ Write( m, Clock::now() ); }
		α Log( const MessageBase& m, vector<string>& values )ι->void override{ Write( m, Clock::now(), &values ); };
		α FetchSessionInfo( SessionPK sessionId )ι->SessionInfoAwait override{ return SessionInfoAwait{sessionId}; }

		α OnDisconnect()ι->void override;
		α OnConnected()ι->void override;
		α SendCustom( uint32 requestId, str bytes )ι->void override;
		α SetCustomFunction( function<Coroutine::Task(uint32,string&&)>&& fnctn )ι{_customFunction=fnctn;}
	private:
		α Write( const MessageBase& message, TimePoint time, vector<string>* pValues=nullptr )ι->void;
		α OnReceive( Logging::Proto::FromServer& fromServer )ι->void override;
		uint _instanceId{0};
		atomic<bool> _stringsLoaded{false};
		string _applicationName;
		function<Coroutine::Task(uint32,string&&)> _customFunction;
		Proto::ToServer _buffer; std::atomic_flag _bufferMutex;
	};
}