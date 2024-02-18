#pragma once
#include <jde/coroutine/Task.h>
#include "ProtoClient.h"
//#include "typedefs.h"
#include "../threading/Interrupt.h"
#include "../collections/Queue.h"
#include "../collections/UnorderedSet.h"

DISABLE_WARNINGS
#include "./proto/messages.pb.h"
ENABLE_WARNINGS

namespace Jde
{
	using ApplicationInstancePK=uint32;
	using ApplicationPK=uint32;
}
namespace Jde::Logging
{
	namespace Messages{ struct ServerMessage; }
	struct IServerSink;
	struct Γ SessionInfoAwait final : IAwait
	{
		SessionInfoAwait( SessionPK sessionId, SRCE )ι:IAwait{sl},_sessionId{sessionId}{}
		α await_suspend( HCoroutine h )ι->void override;
	private:
		SessionPK _sessionId;
	};
#define Φ Γ auto
	namespace Server
	{
		Φ ApplicationId()ι->ApplicationPK;
		α Destroy()ι->void;
		extern bool _enabled;
		Ξ Enabled()ι->bool{ return _enabled; }
		Φ FetchSessionInfo( SessionPK sessionId )ε->SessionInfoAwait;
		Φ InstanceId()ι->ApplicationInstancePK;
		Φ IsLocal()ι->bool;
		Φ Level()ι->ELogLevel; Φ SetLevel( ELogLevel x )ι->void;
		Φ Log( const Messages::ServerMessage& message )ι->void;
		Φ Log( const Messages::ServerMessage& message, vector<string>& values )ι->void;
		Φ Set( sp<Logging::IServerSink>&& p )ι->void;
		α SetSendStatus()ι->void;
		Φ WebSubscribe( ELogLevel level )ι->void;
		Φ Write( Logging::Proto::ToServer&& message )ε->void;
		//Γ α SetServerLevel( ELogLevel serverLevel )ι->void;
	}
#undef Φ
	struct Γ IServerSink{ //: private boost::noncopyable debugging issues
		using ID=uint32;
		IServerSink()ι;
		IServerSink( const unordered_set<ID>& msgs )ι;
		virtual ~IServerSink();

		β ApplicationId()ι->ApplicationPK{return 0;}
		β Close()ι->void=0;
		β InstanceId()ι->ApplicationInstancePK{return _instanceId;}
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
		β SendCustom( ID /*requestId*/, str /*bytes*/ )ι->void{ CRITICALT( AppTag(), "SendCustom not implemented" ); }
		β WebSubscribe( ELogLevel /*level*/ )ι->void{ CRITICALT( AppTag(), "WebSubscribe only for application server" ); }
		β Write( Proto::ToServer&& m )ι->void=0;
	protected:
		ApplicationInstancePK _instanceId;
		UnorderedSet<ID> _messagesSent;
		UnorderedSet<ID> _filesSent;
		UnorderedSet<ID> _functionsSent;
		UnorderedSet<ID> _usersSent;
		UnorderedSet<ID> _threadsSent;
		sp<LogTag> _logTag;
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

	typedef IO::Sockets::TProtoClient<Proto::ToServer,Proto::FromServer> ProtoBase;
	struct ServerSink final: IServerSink, ProtoBase
	{
		using base=ProtoBase;
		static α Create( bool wait = false )ι->Task;
		ServerSink()ε;
		~ServerSink();
		α Close()ι->void override{ ProtoBase::Close(); }
		α Log( Messages::ServerMessage& m )ι->void override{ Write( m, m.Timestamp, &m.Variables ); }
		α Log( const MessageBase& m )ι->void override{ Write( m, Clock::now() ); }
		α Log( const MessageBase& m, vector<string>& values )ι->void override{ Write( m, Clock::now(), &values ); };
		α FetchSessionInfo( SessionPK sessionId )ι->SessionInfoAwait override{ return SessionInfoAwait{sessionId}; }

		α OnDisconnect()ι->void override;
		α OnConnected()ι->void override;
		α SendCustom( uint32 requestId, str bytes )ι->void override;
		α SetCustomFunction( function<Coroutine::Task(uint32,string&&)>&& fnctn )ι{_customFunction=fnctn;}
		α Write( Proto::ToServer&& m )ι->void override{ ProtoBase::Write(move(m)); };
	private:
		α Write( const MessageBase& message, TimePoint time, vector<string>* pValues=nullptr )ι->void;
		α OnReceive( Logging::Proto::FromServer& fromServer )ι->void override;
		atomic<bool> _stringsLoaded{false};
		string _applicationName;
		function<Coroutine::Task(uint32,string&&)> _customFunction;
		Proto::ToServer _buffer; std::atomic_flag _bufferMutex;
	};
}