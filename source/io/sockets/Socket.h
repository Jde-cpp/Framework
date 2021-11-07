#pragma once
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <jde/Exports.h>
#include <jde/App.h>
#include "../../threading/jthread.h"
#include "../../collections/UnorderedMap.h"

namespace Jde::Threading{ struct InterruptibleThread; }

namespace Jde::IO::Sockets
{
	namespace net=boost::asio;
	Γ α LogLevel()noexcept->const LogTag&;
	using SessionPK=uint32;
	struct ISession
	{
		const SessionPK Id;
	};

	struct Γ IOContextThread final //: IShutdown
	{
		static sp<IOContextThread> Instance()noexcept;
		net::io_context& Context()noexcept{ return _ioc; }
		~IOContextThread(){ DBG( "~IOContextThread" ); _ioc.stop(); _thread.join(); }
	private:
		IOContextThread()noexcept;
		void Run()noexcept;
		net::io_context _ioc;
		net::executor_work_guard<boost::asio::io_context::executor_type> _keepAlive;
		std::thread _thread;
		static constexpr sv ThreadName{ "IOContextThread" };
	};

	struct Γ ISocket
	{
		virtual ~ISocket()=0;
	protected:
		ISocket( str settingsPath, PortType defaultPort )noexcept(false);
		ISocket( PortType port )noexcept:Port{port}{}
		const PortType Port;
	};
	inline ISocket::~ISocket(){}

	struct Γ IServerSocket : ISocket
	{
		virtual ~IServerSocket()=0;
		virtual void RemoveSession( SessionPK id )noexcept{ unique_lock l{_sessionMutex}; _sessions.erase( id ); }
		uint SessionCount()const noexcept{ shared_lock l{_sessionMutex}; return _sessions.size(); }
	protected:
		IServerSocket( PortType port )noexcept:ISocket{port}{}
		std::atomic<SessionPK> _id{0};
		flat_map<SessionPK,sp<ISession>> _sessions; mutable shared_mutex _sessionMutex;
	};
	inline IServerSocket::~IServerSocket(){}

	struct Γ IClientSocket : ISocket
	{
		virtual ~IClientSocket()=0;
	protected:
		IClientSocket( str settingsPath, PortType defaultPort )noexcept(false);
		const string Host;
	};
}