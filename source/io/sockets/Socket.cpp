#include "Socket.h"
#include <iostream>
#include "../../threading/Thread.h"
#include "../../threading/InterruptibleThread.h"
#include "../../Settings.h"

#define var const auto

namespace Jde::IO::Sockets
{
	using std::system_error;
	AsyncSocket::AsyncSocket( sv clientThreadName, str settingsPath, PortType defaultPort )noexcept(false):
		AsyncSocket( Settings::TryGet<PortType>(settingsPath+"/port").value_or(defaultPort), clientThreadName, Settings::TryGet<string>(settingsPath+"/host").value_or("localhost") )
	{}
/*	AsyncSocket::AsyncSocket( sv clientThreadName, str settingsPath )noexcept(false):
		AsyncSocket( Settings::Get<PortType>(settingsPath+"/port"), clientThreadName, Settings::TryGet<string>(settingsPath+"/host").value_or("localhost") )
	{}*/
	PortType CheckPort( PortType v )noexcept(false)
	{
		THROW_IF( !v, "port==0" );
		return v;
	}
	AsyncSocket::AsyncSocket( PortType port, sv clientThreadName, sv host )noexcept(false):
		ClientThreadName{ clientThreadName },
		Host{ host },
		Port{ CheckPort(port) },
		_thread{ [&](){Run();} }
	{}

	AsyncSocket::~AsyncSocket()
	{
		TRACEX( "{}"sv, "~AsyncSocket"sv );
		_asyncHelper.stop();
		_thread.join();
	}

	void AsyncSocket::Run()noexcept
	{
		while( !_initialized )
			std::this_thread::yield();
		Threading::SetThreadDscrptn( ClientThreadName );
		DBG( "({})Thread - Entering."sv, ClientThreadName );
		_asyncHelper.run();
		DBG( "({})Thread - Leaving."sv, ClientThreadName );
		//OnClose();
	}

/*	void AsyncSocket::Shutdown()noexcept
	{
		_asyncHelper.stop();
	}
	void AsyncSocket::RunAsyncHelper( sv clientThreadName )noexcept
	{
		ASSERT( !_pThread );
		_threadName = clientThreadName;
		_pThread = make_unique<Threading::InterruptibleThread>( _threadName, [&](){Run();} );
	}
*/
/*	void AsyncSocket::Close()noexcept
	{
		_asyncHelper.stop();
		_pThread->Join();
	}
*/
/*	PerpetualAsyncSocket::PerpetualAsyncSocket( sv clientThreadName, str settingsPath )noexcept(false):
		AsyncSocket{ clientThreadName, settingsPath },
		_keepAlive{ boost::asio::make_work_guard(_asyncHelper) }
	{}
*/
	PerpetualAsyncSocket::PerpetualAsyncSocket( sv clientThreadName, str settingsPath, PortType defaultPort )noexcept(false):
		AsyncSocket{ clientThreadName, settingsPath, defaultPort },
		_keepAlive{ boost::asio::make_work_guard(_asyncHelper) }
	{}
}