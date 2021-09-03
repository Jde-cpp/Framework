#pragma once
#include "../../threading/jthread.h"
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <jde/Exports.h>
#include <jde/App.h>

//namespace boost::asio{ class io_context; class executor_work_guard; class executor_type; }
namespace Jde::Threading{ struct InterruptibleThread; }

namespace Jde::IO::Sockets
{
	namespace basio=boost::asio;
	struct JDE_NATIVE_VISIBILITY AsyncSocket //: public IShutdown
	{
		virtual ~AsyncSocket()=0;
	protected:
		AsyncSocket( sv clientThreadName, str settingsPath, PortType defaultPort )noexcept(false);
		basio::io_context _asyncHelper;
		//void Join();
		//void RunAsyncHelper( sv clientThreadName )noexcept;
		//void Close()noexcept;
		//virtual void OnClose()noexcept{};
		const string ClientThreadName;
		const string Host;
		const PortType Port;
	protected:
		atomic<bool> _initialized{false};
	private:
		AsyncSocket( PortType port, sv clientThreadName, sv host )noexcept(false);
		void Run()noexcept;
		std::thread _thread;
	};

	struct PerpetualAsyncSocket : protected AsyncSocket
	{
		//PerpetualAsyncSocket( sv clientThreadName, str settingsPath )noexcept(false);
		PerpetualAsyncSocket( sv clientThreadName, str settingsPath, PortType defaultPort )noexcept(false);
		virtual ~PerpetualAsyncSocket()=default;
	protected:
		//PerpetualAsyncSocket()noexcept;
		basio::executor_work_guard<boost::asio::io_context::executor_type> _keepAlive;
	};
}