#include "Socket.h"
#include <iostream>
#include "../../threading/Thread.h"
#include "../../threading/InterruptibleThread.h"

#define var const auto

namespace Jde::IO::Sockets
{
	using std::system_error;
	AsyncSocket::AsyncSocket()noexcept
	{
		TRACEX( "{}"sv, "AsyncSocket::AsyncSocket"sv );
	}

	AsyncSocket::~AsyncSocket()
	{
		TRACEX( "{}"sv, "~AsyncSocket"sv );
		if( _pThread )
			_pThread->Join();
	}

	void AsyncSocket::Join()
	{
		if( _pThread )
		{
			_pThread->Join();
			_pThread = nullptr;
		}
	}

	void AsyncSocket::Run()noexcept
	{
		TRACE( "Entering {}"sv, _threadName );
		_asyncHelper.run();
		TRACE( "Leaving {}"sv, _threadName );
		OnClose();
	}
	void AsyncSocket::Shutdown()noexcept
	{
		_asyncHelper.stop();
	}
	void AsyncSocket::RunAsyncHelper( string_view clientThreadName )noexcept
	{
		ASSERT( !_pThread );
		_threadName = clientThreadName;
		_pThread = make_unique<Threading::InterruptibleThread>( _threadName, [&](){Run();} );
	}

	void AsyncSocket::Close()noexcept
	{
		_asyncHelper.stop();
		_pThread->Join();
	}

	PerpetualAsyncSocket::PerpetualAsyncSocket()noexcept:
		_keepAlive{ boost::asio::make_work_guard(_asyncHelper) }
	{}
}