#pragma once
#include <boost/asio.hpp>
//#include <jde/Exports.h>

namespace Jde::IO
{
	namespace net = boost::asio;

	struct __declspec( dllexport ) AsioContextThread final //: IShutdown
	{
		static sp<AsioContextThread> Instance()ι;
		net::io_context& Context()ι{ return _ioc; }
		~AsioContextThread(){ TRACET( LogTag(), "~AsioContextThread" ); _ioc.stop(); _thread.join(); }
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