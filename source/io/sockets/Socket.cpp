#include "Socket.h"
#include <iostream>
#include "../../threading/Thread.h"
#include "../../threading/InterruptibleThread.h"
#include "../../Settings.h"

#define var const auto

namespace Jde::IO::Sockets
{
	ELogLevel _logLevel{ ELogLevel::Debug };
	ELogLevel LogLevel()noexcept{ return _logLevel; }
	sp<IOContextThread> _pInstance;
	sp<IOContextThread> IOContextThread::Instance()noexcept
	{
		return _pInstance ?  _pInstance : _pInstance = sp<IOContextThread>( new IOContextThread() );
	}

	IOContextThread::IOContextThread()noexcept:
		_ioc{1},
		_keepAlive{ net::make_work_guard(_ioc) },
		_thread{ [&](){Run();} }
	{}

	void IOContextThread::Run()noexcept
	{
		Threading::SetThreadDscrptn( ThreadName );
		TAG( "Threads", "({})Thread - Entering.", ThreadName );
		boost::system::error_code ec;
		_ioc.run( ec );
		TAG( "Threads", "({})Thread - Leaving - {} - {}.", ThreadName, ec.value(), ec.message() );
	}

	using std::system_error;
	PortType CheckPort( PortType v )noexcept(false)
	{
		THROW_IF( !v, "port==0" );
		return v;
	}

	ISocket::ISocket( str settingsPath, PortType defaultPort )noexcept(false):
		Port{ CheckPort(Settings::TryGet<PortType>(settingsPath+"/port").value_or(defaultPort)) }
	{}

	IClientSocket::IClientSocket( str settingsPath, PortType defaultPort )noexcept(false):
		ISocket{ settingsPath, defaultPort },
		Host{ Settings::TryGet<string>(settingsPath+"/host").value_or("localhost") }
	{
		TRACE( "IClientSocket::IClientSocket( path='{}', Host='{}', Port='{}' )", settingsPath, Host, Port );
	}

	IClientSocket::~IClientSocket()
	{
		LOG( LogLevel(), "IClientSocket::~IClientSocket" );
	}


}