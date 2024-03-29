﻿#include "Socket.h"
#include <iostream>
#include "../threading/Thread.h"
#include "../threading/InterruptibleThread.h"
#include "../Settings.h"

#define var const auto

namespace Jde::IO::Sockets
{
	static var _logTag{ Logging::Tag("net") };
	α LogTag()ι->sp<Jde::LogTag>{ return _logTag; }

	using std::system_error;
	PortType CheckPort( PortType v )ε
	{
		THROW_IF( !v, "port==0" );
		return v;
	}

	ISocket::ISocket( str settingsPath, PortType defaultPort )ε:
		Port{ CheckPort(Settings::Get<PortType>(settingsPath+"/port").value_or(defaultPort)) }
	{}

	IClientSocket::IClientSocket( str settingsPath, PortType defaultPort )ε:
		ISocket{ settingsPath, defaultPort },
		Host{ Settings::Get<string>(settingsPath+"/host").value_or("localhost") }
	{
		DBG( "IClientSocket::IClientSocket( path='{}', Host='{}', Port='{}' )", settingsPath, Host, Port );
	}

	IClientSocket::~IClientSocket()
	{
		TRACE( "IClientSocket::~IClientSocket" );
	}
}