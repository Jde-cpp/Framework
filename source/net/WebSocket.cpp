#include "WebSocket.h"
#include <jde/Exception.h>

#define var const auto
namespace Jde::WebSocket
{
	static const LogTag& _logLevel = Logging::TagLevel( "webRequests" );
	const LogTag& Session::_logLevel = Logging::TagLevel( "net" );;
	WebListener::WebListener( PortType port )ε:
		IServerSocket{ port },
		_pContextThread{ IOContextThread::Instance() },
		_acceptor{ _pContextThread->Context() }
	{
		try
		{
			const tcp::endpoint ep{ boost::asio::ip::tcp::v4(), ISocket::Port };
			_acceptor.open( ep.protocol() );
			_acceptor.set_option( net::socket_base::reuse_address(true) );
			_acceptor.bind( ep );
			_acceptor.listen( net::socket_base::max_listen_connections );
			DoAccept();
		}
		catch( boost::system::system_error& e )
		{
			throw CodeException( e.what(), e.code() );
		}
	}

#define CHECK_EC(ec, ...) if( ec ){ CodeException x(ec __VA_OPT__(,) __VA_ARGS__); return; }
	void WebListener::DoAccept()ι
	{
		_acceptor.async_accept( net::make_strand(_pContextThread->Context()), [this]( beast::error_code ec, tcp::socket socket )ι
			{
				if( /*ec.value() == 125 &&*/ IApplication::ShuttingDown() )
					return INFO("Webserver shutdown");
				CHECK_EC( ec );
				var id = ++_id;
				sp<ISession> pSession = CreateSession( *this, id, move(socket) );//deadlock if included in _sessions.emplace
				unique_lock l{ _sessionMutex };
				_sessions.emplace( id, pSession );
				DoAccept();
			} );
	}

	void Session::Run()ι
	{
		LOG( "({})Session::Run()", Id );
		net::dispatch( _ws.get_executor(), beast::bind_front_handler(&Session::OnRun, shared_from_this()) );
	}

	void Session::OnRun()ι
	{
		LOG( "({})Session::OnRun()", Id );
		_ws.set_option( websocket::stream_base::timeout::suggested(beast::role_type::server) );
		_ws.set_option( websocket::stream_base::decorator([]( websocket::response_type& res )
			{
				res.set( http::field::server, string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async" );
			}) );
		_ws.async_accept( beast::bind_front_handler(&Session::OnAccept,shared_from_this()) );
	}
	void Session::OnAccept( beast::error_code ec )ι
	{
		LOG( "({})Session::OnAccept()", Id );
		CHECK_EC( ec );
		DoRead();
	}
	void Session::DoRead()ι
	{
		LOG( "({})Session::DoRead()", Id );
		_ws.async_read( _buffer, [this]( beast::error_code ec, uint c )ι
			{
				boost::ignore_unused( c );
				CHECK_EC( ec, ec == websocket::error::closed ? LogLevel().Level : ELogLevel::Error );
				LOG( "({})Session::DoRead({})", Id, c );
				OnRead( (char*)_buffer.data().data(), _buffer.size() );
				DoRead();
			} );
	}

	void Session::OnWrite( beast::error_code ec, uint c )ι
	{
		boost::ignore_unused( c );
		try
		{
			THROW_IFX( ec, CodeException(ec, ec == websocket::error::closed ? LogLevel().Level : ELogLevel::Error) );
		}
		catch( const CodeException& )
		{}
	}
}
