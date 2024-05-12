#pragma once
#ifndef CONTEXT_THREAD_H
#define CONTEXT_THREAD_H
#include <boost/asio.hpp>


namespace Jde::IO
{
	namespace net = boost::asio;

	struct Γ AsioContextThread final //: IShutdown
	{
		static sp<AsioContextThread> Instance()ι;
		net::io_context& Context()ι{ return _ioc; }
		~AsioContextThread();
	private:
		auto LogTag()ι->sp<Jde::LogTag>;
		AsioContextThread()ι;
		void Run()ι;
		net::io_context _ioc;
		net::executor_work_guard<boost::asio::io_context::executor_type> _keepAlive;
		std::thread _thread;
		static constexpr sv ThreadName{ "AsioContextThread" };
	};
}
#endif