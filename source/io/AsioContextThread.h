#pragma once
#include <boost/asio.hpp>

namespace Jde::IO
{
	namespace net = boost::asio;

	struct Γ AsioContextThread final //: IShutdown
	{
		static sp<AsioContextThread> Instance()noexcept;
		net::io_context& Context()noexcept{ return _ioc; }
		~AsioContextThread(){ DBG( "~AsioContextThread" ); _ioc.stop(); _thread.join(); }
	private:
		AsioContextThread()noexcept;
		void Run()noexcept;
		net::io_context _ioc;
		net::executor_work_guard<boost::asio::io_context::executor_type> _keepAlive;
		std::thread _thread;
		static constexpr sv ThreadName{ "AsioContextThread" };
	};
}