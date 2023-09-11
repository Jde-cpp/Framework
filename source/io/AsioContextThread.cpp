#include "AsioContextThread.h"
#include <jde/App.h>
#include "../threading/Thread.h"

#define var const auto
namespace Jde::IO
{
	static var _logLevel{ Logging::TagLevel("sockets") };
	α LogLevel()noexcept->const LogTag&{ return _logLevel; }

	sp<AsioContextThread> _pInstance;
	sp<AsioContextThread> AsioContextThread::Instance()noexcept
	{
		return _pInstance ?  _pInstance : _pInstance = sp<AsioContextThread>( new AsioContextThread() );
	}

	AsioContextThread::AsioContextThread()noexcept:
		_ioc{1},
		_keepAlive{ net::make_work_guard(_ioc) },
		_thread{ [&](){Run();} }
	{}

	void AsioContextThread::Run()noexcept
	{
		Threading::SetThreadDscrptn( ThreadName );
		LOG( "({})Thread - Entering.", ThreadName );
		boost::system::error_code ec;
		_ioc.run( ec );
		LOG( "({})Thread - Leaving - {} - {}.", ThreadName, ec.value(), ec.message() );
	}
}