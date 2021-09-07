#pragma once
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <jde/Exports.h>
#include <jde/App.h>
#include "../../threading/jthread.h"
#include "../../collections/UnorderedMap.h"

//namespace boost::asio{ class io_context; class executor_work_guard; class executor_type; }
namespace Jde::Threading{ struct InterruptibleThread; }

namespace Jde::IO::Sockets
{
	namespace net=boost::asio;
	ELogLevel LogLevel()noexcept;
	using SessionPK=uint;
	struct ISession
	{
		//void Write( string&& data )noexcept;
		const SessionPK Id;

	};

	struct JDE_NATIVE_VISIBILITY IOContextThread final //: IShutdown
	{
		IOContextThread()noexcept;
		//void Shutdown()noexcept override;
		static net::io_context& GetContext()noexcept;
	private:
		void Run()noexcept;
		net::io_context _ioc;
		net::executor_work_guard<boost::asio::io_context::executor_type> _keepAlive;
		std::thread _thread;
		static constexpr sv ThreadName{ "IOContextThread" };
	};

	struct JDE_NATIVE_VISIBILITY ISocket
	{
		virtual ~ISocket()=0;
	protected:
		ISocket( str settingsPath, PortType defaultPort )noexcept(false);
		ISocket( PortType port )noexcept:Port{port}{}
		const PortType Port;
	};
	inline ISocket::~ISocket(){}

	struct JDE_NATIVE_VISIBILITY IServerSocket : ISocket
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

	struct JDE_NATIVE_VISIBILITY IClientSocket : ISocket
	{
		virtual ~IClientSocket()=0;
	protected:
		IClientSocket( str settingsPath, PortType defaultPort )noexcept(false);
		//net::io_context _asyncHelper;
		//const string ClientThreadName;
		const string Host;
	//protected:
	//	atomic<bool> _initialized{false};
	private:
	//	AsyncSocket( PortType port, sv clientThreadName, sv host )noexcept(false);
	//	void Run()noexcept;
	//	std::thread _thread;
	};

/*	struct PerpetualAsyncSocket : protected AsyncSocket
	{
		PerpetualAsyncSocket( sv clientThreadName, str settingsPath, PortType defaultPort )noexcept(false);
		virtual ~PerpetualAsyncSocket()=default;
	protected:
		net::executor_work_guard<boost::asio::io_context::executor_type> _keepAlive;
	};
*/
}