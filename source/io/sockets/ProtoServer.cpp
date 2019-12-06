#include "ProtoServer.h"
#include "../../threading/Thread.h"
#include "../../threading/InterruptibleThread.h"

namespace Jde::IO::Sockets
{
	ProtoServer::ProtoServer( PortType port )noexcept(false)
	{
		auto endpoint = basio::ip::tcp::endpoint( basio::ip::tcp::v4(), port );
		_pAcceptor = make_unique<basio::ip::tcp::acceptor>( _asyncHelper, endpoint ); //"bind: Address already in use
	}
	
	ProtoServer::~ProtoServer()
	{
	/*	TODO
		auto onClose = [this]()
		{
			try
			{
				//if( _pAcceptor )
				//	_pAcceptor->close(); 
			}
			catch( const std::system_error& e )
			{
				GetDefaultLogger()->error( "Server::~Server({})", e.what() );
			}
		};*/
		//_pAcceptor->get_io_service().post( onClose ); boost 1_71 https://github.com/rstudio/rstudio/issues/4636
	}

	void ProtoServer::Close()noexcept
	{
		_pAcceptor->close();
		AsyncSocket::Close();
	}

	vector<google::protobuf::uint8> data{4,0};
	void Test( basio::ip::tcp::socket socket )
	{
		auto onDone = [&]( std::error_code ec, std::size_t length )
		{
			if( ec )
				ERR0( CodeException::ToString(ec) );
			else
				DBG("length={}", length);
		};
		basio::async_write( socket, basio::buffer(data.data(), data.size()), onDone );
	}
	void ProtoServer::Accept()noexcept
	{
		auto onAccept = [this]( std::error_code ec, basio::ip::tcp::socket socket )
		{
			if( ec )
			{
				if( ec.value()==125 )
					Jde::GetDefaultLogger()->info( "Sever shutting down - {}", CodeException::ToString(ec) );
				else
					Jde::GetDefaultLogger()->error( "Accepted Failed - {}", CodeException::ToString(ec) );
			}
			else
			{
				// auto onDone = [&]( std::error_code ec, std::size_t length )
				// {
				// 	if( ec )
				// 		ERR0( CodeException::ToString(ec) );
				// 	else
				// 		DBG("length={}", length);
				// };
				
				var id = ++_id;
				DBG( "Accepted Connection - {}", id );
//				basio::async_write( socket, basio::buffer(data.data(), data.size()), onDone );
				//Test( std::move(socket) );
				auto pSession = CreateSession( std::move(socket), id );
				if( pSession )
					_sessions.emplace( id, pSession );
				Accept();
			}
		};
		_pAcceptor->async_accept( onAccept );
	}

	//	virtual ISession OnSessionStart( SessionPK id )noexcept=0;
	void ProtoServer::Run()noexcept
	{
		Threading::SetThreadDescription( "ProtoServer::Run" );
		DBG0( "ProtoServer::Run" );		
		_asyncHelper.run();
		DBG0( "ProtoServer::Run Exit" );		
	}

	ProtoSession::ProtoSession( basio::ip::tcp::socket& socket, SessionPK id )noexcept:
		Id{ id }, 
		_socket2( std::move(socket) )
	{
		ReadHeader();
	}

	void ProtoSession::ReadHeader()noexcept
	{
		auto onComplete = [&]( std::error_code ec, uint headerLength )
		{
			if( ec )
			{
				if( ec.value()==2 || ec.value()==10054 )
				{
					TRACE( "disconnection - '{}'", CodeException::ToString(ec) );
					OnDisconnect();
				}
				else
					ERR0( CodeException::ToString(ec) );
			}
			else if( headerLength!=4 )
				ERR( "only read '{}'", headerLength );
			else
			{
				var messageLength = ProtoClientSession::MessageLength( _readMessageSize );
				TRACE( "messageLength: {}", messageLength );
				ReadBody( messageLength );
			}
		};
		TRACE0( "Server::Session::ReadHeader" );
		basio::async_read( _socket2, basio::buffer(static_cast<void*>(_readMessageSize), sizeof(_readMessageSize)), onComplete );
	}
}