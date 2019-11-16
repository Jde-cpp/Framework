#include "stdafx.h"
#include "Socket.h"
#include <iostream>
#include "../../threading/Thread.h"
#include "../../threading/InterruptibleThread.h"
//#include "SessionCallbacks.h"

#define var const auto

namespace Jde::IO::Sockets
{
	using std::system_error;
	AsyncSocket::AsyncSocket()noexcept
	{
		TRACEX( "{}", "AsyncSocket::AsyncSocket" );
	}
	
	AsyncSocket::~AsyncSocket()
	{
		TRACEX( "{}", "~AsyncSocket" );
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
//		Threading::SetThreadDescription( _threadName );
		TRACE( "Entering {}", _threadName );
		_asyncHelper.run();
		TRACE( "Leaving {}", _threadName );
		OnClose();
	}
	void AsyncSocket::Shutdown()noexcept
	{
		_asyncHelper.stop();
	}
	void AsyncSocket::RunAsyncHelper()noexcept
	{
		ASSERT( !_pThread );
		_threadName = "AsyncSocket";
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
	/*
	uint_fast32_t Session::MessageLength()const noexcept
	{
		uint32_t length;
		char* pDestination = reinterpret_cast<char*>( &length );
		const char* pStart = _readMessageSize;
		std::copy( pStart, pStart+4, pDestination );
		return ntohl( length );
	}
	*/
}