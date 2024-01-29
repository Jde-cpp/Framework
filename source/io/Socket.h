#pragma once
#include <thread>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <jde/Exports.h>
#include <jde/App.h>
//#include "../../threading/jthread.h"
#include "../collections/UnorderedMap.h"

namespace Jde::Threading{ struct InterruptibleThread; }

namespace Jde::IO::Sockets
{
	namespace net=boost::asio;
	Γ α LogTag()ι->sp<Jde::LogTag>;
	using SessionPK=uint32;
	struct ISession
	{
		ISession( SessionPK id ):Id{id}{}
		virtual ~ISession()=0;
		const SessionPK Id;
	};
	inline ISession::~ISession(){}

	struct Γ ISocket
	{
		virtual ~ISocket()=0;
	protected:
		ISocket( str settingsPath, PortType defaultPort )ι(false);
		ISocket( PortType port )ι:Port{port}{}
		const PortType Port;
	};
	inline ISocket::~ISocket(){}

	struct Γ IServerSocket : ISocket
	{
		virtual ~IServerSocket()=0;
		virtual void RemoveSession( SessionPK id )ι{ unique_lock l{_sessionMutex}; _sessions.erase( id ); }
		uint SessionCount()const ι{ shared_lock l{_sessionMutex}; return _sessions.size(); }
	protected:
		IServerSocket( PortType port )ι:ISocket{port}{}
		std::atomic<SessionPK> _id{0};
		flat_map<SessionPK,sp<ISession>> _sessions; mutable shared_mutex _sessionMutex;
	};
	inline IServerSocket::~IServerSocket(){}

	struct Γ IClientSocket : ISocket
	{
		virtual ~IClientSocket()=0;
	protected:
		IClientSocket( str settingsPath, PortType defaultPort )ι(false);
		const string Host;
	};
}