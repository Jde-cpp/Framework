#pragma once
#ifndef CONTEXT_THREAD_H
#define CONTEXT_THREAD_H
#include <boost/asio.hpp>

#define Φ Γ α
namespace Jde::IO{

	Φ AsioContextThread( uint8 threadCount=1 )ι->sp<boost::asio::io_context>;
	Φ DeleteContextThread()ι->void;
	/*
	struct Γ AsioContextThread final{ //: IShutdown
		static sp<AsioContextThread> Instance()ι;


		~AsioContextThread();
	private:
		auto LogTag()ι->sp<Jde::LogTag>;
		AsioContextThread()ι;
		void Run()ι;
		sp<net::io_context> _ioc;
		net::executor_work_guard<boost::asio::io_context::executor_type> _keepAlive;
		std::thread _thread;
		static constexpr sv ThreadName{ "AsioContextThread" };
	};
	*/
}
#endif
#undef Φ