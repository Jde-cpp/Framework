#include "ProtoServer.h"
#include "../../threading/Thread.h"
#include "../../threading/InterruptibleThread.h"
#include "../../Settings.h"

#define var const auto
namespace Jde::IO::Sockets
{
	ProtoServer::ProtoServer( sv clientThreadName, str settingsPath, PortType defaultPort )noexcept(false):
		PerpetualAsyncSocket{ clientThreadName, settingsPath, defaultPort },
		_acceptor{ _asyncHelper, basio::ip::tcp::endpoint{basio::ip::tcp::v4(), AsyncSocket::Port } }
	{
		LOG_MEMORY( ELogLevel::Information, string{"({}) Accepting on port '{}'"}, ClientThreadName, Port );
		_initialized = true;
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

/*	void ProtoServer::Close()noexcept
	{
		_acceptor.close();
		AsyncSocket::Close();
	}
*/
	void ProtoServer::Accept()noexcept
	{
		_acceptor.async_accept( [this]( std::error_code ec, basio::ip::tcp::socket socket )noexcept
		{
			try
			{
				THROW_IFX( ec, CodeException(ec.value()==125 ? "Sever shutting down"sv : "Accept Failed"sv, move(ec), ec.value()==125 ? ELogLevel::Information : ELogLevel::Error) );
				var id = ++_id;
				DBG( "({})Accepted Connection"sv, id );
				unique_lock l{ _mutex };
				_sessions.emplace( id, CreateSession(std::move(socket), id) );
				Accept();
			}
			catch( const Exception& ){}
		});
	}

	//	virtual ISession OnSessionStart( SessionPK id )noexcept=0;
/*	void ProtoServer::Run()noexcept
	{
		Threading::SetThreadDscrptn( "ProtoServer::Run" );
		DBG( "ProtoServer::Run"sv );
		_asyncHelper.run();
		DBG( "ProtoServer::Run Exit"sv );
	}
*/
	ProtoSession::ProtoSession( basio::ip::tcp::socket&& socket, SessionPK id )noexcept:
		Id{ id },
		_socket( std::move(socket) )
	{
		ReadHeader();
	}

	void ProtoSession::ReadHeader()noexcept
	{
		LOG( _logLevel, "ProtoSession::ReadHeader" );
		basio::async_read( _socket, basio::buffer(static_cast<void*>(_readMessageSize), sizeof(_readMessageSize)), [&]( std::error_code ec, uint headerLength )
		{
			try
			{
				THROW_IFX( ec, CodeException(move(ec), ec.value()==2 || ec.value()==10054 || ec.value()==104 ? _logLevel : ELogLevel::Error) );
				THROW_IF( headerLength!=4, "only read '{}'"sv, headerLength );

				var messageLength = ProtoClientSession::MessageLength( _readMessageSize );
				LOG( _logLevel, "messageLength: {}", messageLength );
				ReadBody( messageLength );
			}
			catch( const Exception& e )
			{
				if( e.GetLevel()<ELogLevel::Error )
					OnDisconnect();
			}
		});
	}
}