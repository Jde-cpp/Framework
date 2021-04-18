#pragma once
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include "../../Exports.h"
#include "../../application/Application.h"

//namespace boost::asio{ class io_context; class executor_work_guard; class executor_type; }
namespace Jde::Threading{ struct InterruptibleThread; }

namespace Jde::IO::Sockets
{
	namespace basio=boost::asio;
	struct JDE_NATIVE_VISIBILITY AsyncSocket : public IShutdown
	{
		virtual ~AsyncSocket();
	protected:
		AsyncSocket()noexcept;
		basio::io_context _asyncHelper;
		void Join();
		void RunAsyncHelper( sv clientThreadName )noexcept;
		void Close()noexcept;
		virtual void OnClose()noexcept{};
		void Shutdown()noexcept;
	private:
		void Run()noexcept;
		sp<Threading::InterruptibleThread> _pThread;
		string _threadName;
	};

	class PerpetualAsyncSocket : protected AsyncSocket
	{
	public:
		virtual ~PerpetualAsyncSocket()=default;
	protected:
		PerpetualAsyncSocket()noexcept;
		basio::executor_work_guard<boost::asio::io_context::executor_type> _keepAlive;
	};
}