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

namespace Jde::Logging
{
	struct IServerSink;
	namespace Messages{ struct ServerMessage; }
	Γ α Server()noexcept->up<Logging::IServerSink>&; Γ α SetServer( up<Logging::IServerSink> p )noexcept->void;
	Γ α ServerLevel()noexcept->ELogLevel; Γ α SetServerLevel( ELogLevel serverLevel )noexcept->void;

	struct Γ IServerSink : private boost::noncopyable
	{
		using ID=uint32;
		IServerSink()noexcept=default;
		IServerSink( const unordered_set<ID>& msgs )noexcept:_messagesSent{msgs}{}
		virtual ~IServerSink();

		β Log( Messages::ServerMessage& message )noexcept->void=0;
		β Log( const MessageBase& messageBase )noexcept->void=0;
		β Log( const MessageBase& messageBase, vector<string>& values )noexcept->void=0;

		α ShouldSendMessage( ID messageId )noexcept->bool{ return _messagesSent.emplace(messageId); }
		α ShouldSendFile( ID messageId )noexcept->bool{ return _filesSent.emplace(messageId); }
		α ShouldSendFunction( ID messageId )noexcept->bool{ return _functionsSent.emplace(messageId); }
		α ShouldSendUser( ID messageId )noexcept->bool{ return _usersSent.emplace(messageId); }
		α ShouldSendThread( ID messageId )noexcept->bool{ return _threadsSent.emplace(messageId); }
		β SendCustom( ID /*requestId*/, str /*bytes*/ )noexcept->void{ CRITICAL("SendCustom not implemented"); }
		Ω Enabled()noexcept->bool{ return _enabled; }
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
			ServerMessage( const MessageBase& base )noexcept:
				Logging::Message{ base }
			{}
			ServerMessage( const ServerMessage& base );
			ServerMessage( const MessageBase& base, vector<string> values )noexcept;
			ServerMessage( const Message& b, vector<string> values )noexcept:
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
		static α Create()noexcept->ServerSink*;
		ServerSink()noexcept(false);
		~ServerSink(){DBGX("{}"sv, "~ServerSink");}
		α Log( Messages::ServerMessage& m )noexcept->void override{ Write( m, m.Timestamp, &m.Variables ); }
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
		Proto::ToServer _buffer; std::atomic_flag _bufferMutex;
	};
}