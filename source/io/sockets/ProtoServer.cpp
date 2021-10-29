#include "ProtoServer.h"
#include "../../threading/Thread.h"
#include "../../threading/InterruptibleThread.h"
#include "../../Settings.h"

#define var const auto
namespace Jde::IO::Sockets
{
	ProtoServer::ProtoServer( PortType port )noexcept:
		ISocket{ port },
		_pIOContext{ IOContextThread::Instance() },
		_acceptor{ _pIOContext->Context(), tcp::endpoint{tcp::v4(), port} }
	{}

	ProtoServer::~ProtoServer()
	{}

	void ProtoServer::Accept()noexcept
	{
		_acceptor.async_accept( [this]( std::error_code ec, tcp::socket socket )noexcept
		{
			try
			{
				THROW_IFX2( ec, CodeException(ec.value()==125 ? "Sever shutting down"sv : "Accept Failed"sv, move(ec), ec.value()==125 ? ELogLevel::Information : ELogLevel::Error) );
				var id = ++_id;
				DBG( "({})Accepted Connection"sv, id );
				unique_lock l{ _mutex };
				_sessions.emplace( id, CreateSession(std::move(socket), id) );
				Accept();
			}
			catch( const IException& ){}
		});
	}

	ProtoSession::ProtoSession( tcp::socket&& socket, SessionPK id )noexcept:
		Id{ id },
		_socket( std::move(socket) )
	{
		ReadHeader();
	}

	void ProtoSession::ReadHeader()noexcept
	{
		net::async_read( _socket, net::buffer(static_cast<void*>(_readMessageSize), sizeof(_readMessageSize)), [&]( std::error_code ec, uint headerLength )
		{
			try
			{
				THROW_IFX2( ec && ec.value()==2, CodeException(fmt::format("({}) - Disconnected", Id), move(ec), _logLevel) );
				THROW_IFX2( ec, CodeException(move(ec), ec.value()==10054 || ec.value()==104 ? _logLevel : ELogLevel::Error) );
				THROW_IF( headerLength!=4, "only read '{}'"sv, headerLength );

				var messageLength = ProtoClientSession::MessageLength( _readMessageSize );
				ReadBody( messageLength );
			}
			catch( const IException& e )
			{
				if( e.Level()<ELogLevel::Error )
					OnDisconnect();
			}
		});
	}
}