#include "Socket.h"
#include <iostream>
#include "../threading/Thread.h"
#include "../threading/InterruptibleThread.h"
#include "../Settings.h"

#define var const auto

namespace Jde::IO::Sockets
{
	static var _logLevel{ Logging::TagLevel("sockets") };
	α LogLevel()noexcept->const LogTag&{ return _logLevel; }

	using std::system_error;
	PortType CheckPort( PortType v )noexcept(false)
	{
		THROW_IF( !v, "port==0" );
		return v;
	}

	ISocket::ISocket( str settingsPath, PortType defaultPort )noexcept(false):
		Port{ CheckPort(Settings::Get<PortType>(settingsPath+"/port").value_or(defaultPort)) }
	{}

	IClientSocket::IClientSocket( str settingsPath, PortType defaultPort )noexcept(false):
		ISocket{ settingsPath, defaultPort },
		Host{ Settings::Get<string>(settingsPath+"/host").value_or("localhost") }
	{
		TRACE( "IClientSocket::IClientSocket( path='{}', Host='{}', Port='{}' )", settingsPath, Host, Port );
	}

	IClientSocket::~IClientSocket()
	{
		LOG( "IClientSocket::~IClientSocket" );
	}
}