#include "AsioContextThread.h"
#include <jde/App.h>
#include "../threading/Thread.h"
#include <jde/TypeDefs.h>

#define var const auto
namespace Jde::IO
{
	static var _logTag{ Logging::Tag("sockets") };
	auto AsioContextThread::LogTag()ι->sp<Jde::LogTag>{ return _logTag; }

	sp<AsioContextThread> _pInstance;
	sp<AsioContextThread> AsioContextThread::Instance()ι{
		return _pInstance ?  _pInstance : _pInstance = sp<AsioContextThread>( new AsioContextThread{} );
	}

	AsioContextThread::AsioContextThread()ι:
		_ioc{1},
		_keepAlive{ net::make_work_guard(_ioc) },
		_thread{ [&](){Run();} }
	{}
	
	AsioContextThread::~AsioContextThread(){
		TRACET( LogTag(), "~AsioContextThread" );
		_ioc.stop();
		if( _thread.joinable() )
			_thread.join();
	}

	void AsioContextThread::Run()ι{
		Threading::SetThreadDscrptn( ThreadName );
		TRACE( "({})Thread - Entering.", ThreadName );
		boost::system::error_code ec;
		_ioc.run( ec );
		TRACE( "({})Thread - Leaving - {} - {}.", ThreadName, ec.value(), ec.message() );
	}
}