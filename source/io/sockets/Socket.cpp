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
	up<IOContextThread> _pInstance;
	net::io_context& IOContextThread::GetContext()noexcept
	{
		if( !_pInstance )
			_pInstance = make_unique<IOContextThread>();
		return _pInstance->_ioc;
	}

	IOContextThread::IOContextThread()noexcept:
		_ioc{1},
		_keepAlive{ net::make_work_guard(_ioc) },
		_thread{ [&](){Run();} }
	{
		LOG( LogLevel(), "IOContextThread::IOContextThread" );
	}

/*	void IOContextThread::Shutdown()noexcept
	{
		DBG( "({}) - Shutdown.", ThreadName );
		_ioc.stop();
		DBG( "({}) - ~Shutdown.", ThreadName );
	}*/

	void IOContextThread::Run()noexcept
	{
		Threading::SetThreadDscrptn( ThreadName );
		LOG( LogLevel(), "({})Thread - Entering."sv, ThreadName );
		boost::system::error_code ec;
		_ioc.run( ec );
		LOG( LogLevel(), "({})Thread - Leaving - {} - {}."sv, ThreadName, ec.value(), ec.message() );
	}

	using std::system_error;
	PortType CheckPort( PortType v )noexcept(false)
	{
		THROW_IF( !v, "port==0" );
		return v;
	}

	ISocket::ISocket( str settingsPath, PortType defaultPort )noexcept(false):
		Port{ CheckPort(Settings::TryGet<PortType>(settingsPath+"/port").value_or(defaultPort)) }
	{
//		LOG( LogLevel(), "IClientSocket::IClientSocket( path='{}', Port='{}' )", settingsPath, Port );
	}

	IClientSocket::IClientSocket( str settingsPath, PortType defaultPort )noexcept(false):
		ISocket{ settingsPath, defaultPort },
		Host{ Settings::TryGet<string>(settingsPath+"/host").value_or("localhost") }
	{
		LOG( LogLevel(), "IClientSocket::IClientSocket( path='{}', Host='{}', Port='{}' )", settingsPath, Host, Port );
	}
/*	ISocket::ISocket( sv clientThreadName, str settingsPath )noexcept(false):
		ISocket( Settings::Get<PortType>(settingsPath+"/port"), clientThreadName, Settings::TryGet<string>(settingsPath+"/host").value_or("localhost") )
	{}*/

	IClientSocket::~IClientSocket()
	{
		LOG( LogLevel(), "IClientSocket::~IClientSocket" );
//		_asyncHelper.stop();
//		_thread.join();
	}

/*	void AsyncSocket::Run()noexcept
	{
		while( !_initialized )
			std::this_thread::yield();
		Threading::SetThreadDscrptn( ClientThreadName );
		DBG( "({})Thread - Entering."sv, ClientThreadName );
		_asyncHelper.run();
		DBG( "({})Thread - Leaving."sv, ClientThreadName );
		//OnClose();
	}
*/
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

	PerpetualAsyncSocket::PerpetualAsyncSocket( sv clientThreadName, str settingsPath, PortType defaultPort )noexcept(false):
		AsyncSocket{ clientThreadName, settingsPath, defaultPort },
		_keepAlive{ boost::asio::make_work_guard(_asyncHelper) }
	{}
*/
}