#include "RestServer.h"
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#define var const auto

namespace Jde::IO::Rest
{

	const LogTag& _logLevel = Logging::TagLevel( "rest" );
#define CHECK_EC(ec, ...) if( ec ){ CodeException x(ec __VA_OPT__(,) __VA_ARGS__); return; }

	IListener::IListener( PortType port )ε:
		_pIOContext{ Sockets::IOContextThread::Instance() },
		_acceptor{ _pIOContext->Context(), tcp::endpoint{ Settings::Get("net/ip").value_or("v6")=="v6" ? tcp::v6() : tcp::v4(), port } }
  {
    beast::error_code ec;
    // Acceptor.open( endpoint.protocol(), ec ); THROW_IF( ec, "open" );
    // Acceptor.set_option(net::socket_base::reuse_address(true), ec); THROW_IF( ec, "reuse_address" );
    // Acceptor.bind(endpoint, ec); THROW_IF( ec, "bind" );
		INFO( "Listening to Rest calls on port '{}'", port );
    _acceptor.listen( net::socket_base::max_listen_connections, ec ); THROW_IF( ec, "listen" );
  }

  α IListener::DoAccept()ι->void
	{
		sp<IListener> sp_ = static_pointer_cast<IListener>( MakeShared() );
		_acceptor.async_accept( net::make_strand(_pIOContext->Context()), beast::bind_front_handler(&IListener::OnAccept, sp_) );
	}

  α IListener::OnAccept( beast::error_code ec, tcp::socket socket )ι->void
  {
		LOG( "ISession::OnAccept()" );
		CHECK_EC( ec );
		CreateSession( std::move(socket) )->Run();
    DoAccept();
  }

	α ISession::DoRead()ι->void
  {
    Request = {};
    Stream.expires_after( std::chrono::seconds(30) );
    http::async_read( Stream, Buffer, Request, beast::bind_front_handler( &ISession::OnRead, MakeShared()) );
  }

  α ISession::OnRead( beast::error_code ec, std::size_t bytes_transferred )ι->void
  {
    boost::ignore_unused(bytes_transferred);
    if(ec == http::error::end_of_stream)
      return DoClose();

		CHECK_EC( ec );

		HandleRequest( std::move(Request), MakeShared() );
  }

	α ISession::OnWrite( bool close, beast::error_code ec, std::size_t bytes_transferred )ι->void
  {
      boost::ignore_unused(bytes_transferred);
			CHECK_EC( ec );
      DoClose();
  }

	α ISession::DoClose()ι->void
  {
      beast::error_code ec;
      Stream.socket().shutdown(tcp::socket::shutdown_send, ec);
  }
	α SetResponse( http::response<http::string_body>& res, int size, bool keepAlive )ι->void
	{
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "application/json");
		res.set(http::field::access_control_allow_origin, "*");
		//https://www.boost.org/doc/libs/1_76_0/boost/beast/http/field.hpp
		//res.content_length( size );
		res.keep_alive( keepAlive );
	}
	α ISession::Send( string value, http::request<http::string_body>&& req )ι->void
	{
		var size = value.size();
		http::response<http::string_body> res{ std::piecewise_construct, std::make_tuple(std::move(value)), std::make_tuple(http::status::ok, req.version()) };
		SetResponse( res, size, req.keep_alive() );
		res.prepare_payload();
		Send(std::move(res));
	}

	α ISession::Error( http::status status, string what, http::request<http::string_body>&& req )ι->void
	{
    http::response<http::string_body> res{status, req.version()};
		var message = format( "{{message: {}}}", move(what) );
		SetResponse( res, message.size(), req.keep_alive() );
    res.body() = message;
    res.prepare_payload();
		Send( move(res) );
	}

/*
	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<
	    class Body, class Allocator,
	    class Send>
	void
	handle_request(
	    beast::string_view doc_root,
	    http::request<Body, http::basic_fields<Allocator>>&& req,
	    Send&& send)
	{
	    // Returns a bad request response
	    auto const bad_request =
	    [&req](beast::string_view why)
	    {
	        http::response<http::string_body> res{http::status::bad_request, req.version()};
	        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	        res.set(http::field::content_type, "text/html");
	        res.keep_alive(req.keep_alive());
	        res.body() = std::string(why);
	        res.prepare_payload();
	        return res;
	    };

	    // Returns a not found response
	    auto const not_found =
	    [&req](beast::string_view target)
	    {
	        http::response<http::string_body> res{http::status::not_found, req.version()};
	        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	        res.set(http::field::content_type, "text/html");
	        res.keep_alive(req.keep_alive());
	        res.body() = "The resource '" + std::string(target) + "' was not found.";
	        res.prepare_payload();
	        return res;
	    };

	    // Returns a server error response
	    auto const server_error =
	    [&req](beast::string_view what)
	    {
	        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
	        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	        res.set(http::field::content_type, "text/html");
	        res.keep_alive(req.keep_alive());
	        res.body() = "An error occurred: '" + std::string(what) + "'";
	        res.prepare_payload();
	        return res;
	    };

	    // Make sure we can handle the method
	    if( req.method() != http::verb::get &&
	        req.method() != http::verb::head)
	        return send(bad_request("Unknown HTTP-method"));

	    // Build the path to the requested file
	    std::string path = path_cat(doc_root, req.target());
	    if(req.target().back() == '/')
	        path.append("index.html");

	    // Attempt to open the file
	    beast::error_code ec;
	    http::file_body::value_type body;
	    body.open(path.c_str(), beast::file_mode::scan, ec);

	    // Handle the case where the file doesn't exist
	    if(ec == beast::errc::no_such_file_or_directory)
	        return send(not_found(req.target()));

	    // Handle an unknown error
	    if(ec)
	        return send(server_error(ec.message()));

	    // Cache the size since we need it after the move
	    auto const size = body.size();

	    // Respond to HEAD request
	    if(req.method() == http::verb::head)
	    {
	        http::response<http::empty_body> res{http::status::ok, req.version()};
	        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	        res.set(http::field::content_type, mime_type(path));
	        res.content_length(size);
	        res.keep_alive(req.keep_alive());
	        return send(std::move(res));
	    }

	    // Respond to GET request
	    http::response<http::file_body> res{
	        std::piecewise_construct,
	        std::make_tuple(std::move(body)),
	        std::make_tuple(http::status::ok, req.version())};
	    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	    res.set(http::field::content_type, mime_type(path));
	    res.content_length(size);
	    res.keep_alive(req.keep_alive());
	    return send(std::move(res));
	}


	//------------------------------------------------------------------------------

	// Accepts incoming connections and launches the sessions
	class listener : public std::enable_shared_from_this<listener>
	{
	    net::io_context& ioc_;
	    tcp::acceptor acceptor_;
	    std::shared_ptr<std::string const> doc_root_;

	public:
	    listener(
	        net::io_context& ioc,
	        tcp::endpoint endpoint,
	        std::shared_ptr<std::string const> const& doc_root)
	        : ioc_(ioc)
	        , acceptor_(net::make_strand(ioc))
	        , doc_root_(doc_root)
	    {
	        beast::error_code ec;

	        // Open the acceptor
	        acceptor_.open(endpoint.protocol(), ec);
	        if(ec)
	        {
	            fail(ec, "open");
	            return;
	        }

	        // Allow address reuse
	        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
	        if(ec)
	        {
	            fail(ec, "set_option");
	            return;
	        }

	        // Bind to the server address
	        acceptor_.bind(endpoint, ec);
	        if(ec)
	        {
	            fail(ec, "bind");
	            return;
	        }

	        // Start listening for connections
	        acceptor_.listen(
	            net::socket_base::max_listen_connections, ec);
	        if(ec)
	        {
	            fail(ec, "listen");
	            return;
	        }
	    }

	    // Start accepting incoming connections
	    void
	    run()
	    {
	        do_accept();
	    }

	private:
	    void
	    do_accept()
	    {
	        // The new connection gets its own strand
	        acceptor_.async_accept(
	            net::make_strand(ioc_),
	            beast::bind_front_handler(
	                &listener::on_accept,
	                shared_from_this()));
	    }

	    void
	    on_accept(beast::error_code ec, tcp::socket socket)
	    {
	        if(ec)
	        {
	            fail(ec, "accept");
	        }
	        else
	        {
	            // Create the session and run it
	            std::make_shared<session>(
	                std::move(socket),
	                doc_root_)->run();
	        }

	        // Accept another connection
	        do_accept();
	    }
	};

	//------------------------------------------------------------------------------

	int main(int argc, char* argv[])
	{
	    // Check command line arguments.
	    if (argc != 5)
	    {
	        std::cerr <<
	            "Usage: http-server-async <address> <port> <doc_root> <threads>\n" <<
	            "Example:\n" <<
	            "    http-server-async 0.0.0.0 8080 . 1\n";
	        return EXIT_FAILURE;
	    }
	    auto const address = net::ip::make_address(argv[1]);
	    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
	    auto const doc_root = std::make_shared<std::string>(argv[3]);
	    auto const threads = std::max<int>(1, std::atoi(argv[4]));

	    // The io_context is required for all I/O
	    net::io_context ioc{threads};

	    // Create and launch a listening port
	    std::make_shared<listener>(
	        ioc,
	        tcp::endpoint{address, port},
	        doc_root)->run();

	    // Run the I/O service on the requested number of threads
	    std::vector<std::thread> v;
	    v.reserve(threads - 1);
	    for(auto i = threads - 1; i > 0; --i)
	        v.emplace_back(
	        [&ioc]
	        {
	            ioc.run();
	        });
	    ioc.run();

	    return EXIT_SUCCESS;
	}
	*/
}