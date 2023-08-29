#pragma once
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <jde/coroutine/Task.h>
#include "Socket.h"
	//https://www.boost.org/doc/libs/1_73_0/libs/beast/example/http/server/async/http_server_async.cpp

namespace Jde::IO::Rest
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	using tcp = boost::asio::ip::tcp;

	struct ISession
	{
		using TMessage = http::message<true, http::string_body, http::basic_fields<std::allocator<char>>>;
    ISession( tcp::socket&& socket ): Stream{move(socket)}//,_send{*this}
    {}

    α Run()ι->void{ net::dispatch( Stream.get_executor(), beast::bind_front_handler( &ISession::DoRead, MakeShared()) ); }
		α DoRead()ι->void;
    α OnRead( beast::error_code ec, std::size_t bytes_transferred )ι->void;
    //template<bool isRequest, class Body, class Fields>
		//α Send( http::message<isRequest, Body, Fields>&& msg )->void;

		α OnWrite( bool close, beast::error_code ec, std::size_t bytes_transferred )ι->void;
		α DoClose()ι->void;
		β HandleRequest( http::request<http::string_body>&& request, sp<ISession> s )ι->void=0;
		α Error( http::status status, string what, http::request<http::string_body>&& req )ι->void;
		α Send( string value, http::request<http::string_body>&& req )ι->void;

		β MakeShared()ι->sp<ISession> = 0;

	private:
	  template<bool isRequest, class Body, class Fields>
		α Send( http::message<isRequest, Body, Fields>&& msg )->void;

    beast::tcp_stream Stream;
    beast::flat_buffer Buffer;
    http::request<http::string_body> Request;
    sp<void> Message;
	};

	struct Γ IListener
	{
	  IListener( PortType defaultPort )ε;
		virtual ~IListener(){}
    α Run()ι->void{ DoAccept(); }
	  α DoAccept()ι->void;
	private:
	  α OnAccept(beast::error_code ec, tcp::socket socket)ι->void;
		β CreateSession( tcp::socket&& socket )ι->sp<ISession> =0;
		virtual auto MakeShared()ι->sp<void> =0;

		sp<Sockets::IOContextThread> _pIOContext;
		tcp::acceptor _acceptor;
	};

	template<class TSession>
	struct TListener : IListener, std::enable_shared_from_this<TListener<TSession>>
	{
	  TListener( PortType port )ε:IListener{ port }{}
		β CreateSession( tcp::socket&& socket )ι->sp<ISession> override;
		auto MakeShared()ι->sp<void> override{ return this->shared_from_this(); }
	};

  template<bool isRequest, class Body, class Fields>
	α ISession::Send( http::message<isRequest, Body, Fields>&& msg )->void
	{
		auto sp = std::make_shared<http::message<isRequest, Body, Fields>>( std::move(msg) );
    Message = sp;
    http::async_write( Stream, *sp, beast::bind_front_handler(&ISession::OnWrite, MakeShared(), sp->need_eof()) );
	}

	template<class TSession>
  α TListener<TSession>::CreateSession( tcp::socket&& socket )ι->sp<ISession>
  {
		return  ms<TSession>( std::move(socket) );
  }

}