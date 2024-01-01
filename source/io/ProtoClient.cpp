#include "ProtoClient.h"
#include "../threading/Thread.h"
#include "../threading/InterruptibleThread.h"
#define var const auto

namespace Jde::IO::Sockets{

#define _logLevel Sockets::LogLevel()

	ProtoClientSession::ProtoClientSession( /*boost::asio::io_context& context*/ ):
		_pIOContext{ AsioContextThread::Instance() },
		_socket{ _pIOContext->Context() }
	{}

	α ProtoClientSession::Close( std::condition_variable* pCvClient )ι->void{
		if( _pIOContext && _socket.is_open() )
			_pWriteKeepAlive = _pReadKeepAlive = shared_from_this();
			DBG( "ProtoClientSession::Close IOContext use_count={}"sv, _pIOContext.use_count() );
			_socket.close();
		//_pIOContext = nullptr;
		if( pCvClient )
			pCvClient->notify_one();
	}

	α ProtoClientSession::MessageLength( char* readMessageSize )ι->uint32{
		uint32_t length;
		char* pDestination = reinterpret_cast<char*>( &length );
		const char* pStart = readMessageSize;
		std::copy( pStart, pStart+4, pDestination );
		return ntohl( length );
	}

	α ProtoClientSession::ReadHeader()ι->void{
		net::async_read( _socket, net::buffer(static_cast<void*>(_readMessageSize), sizeof(_readMessageSize)), [this]( std::error_code ec, std::size_t headerLength )
		{
			if( ec ){
				if( ec.value()==2 )
					DBG( "ProtoClientSession::ReadHeader closing - 2 '{}'", CodeException::ToString(ec) );
				else if( ec.value()==125 )
					DBG( "_socket.close() ec='{}'", CodeException::ToString(ec) );
				else
					DBG( "Client::ReadHeader Failed - ({}){} closing"sv, ec.value(), ec.message() );
				auto keepAlive = _pIOContext;
				if( _pIOContext ){
					if(  _socket.is_open() )
						_socket.close();
					_pIOContext = nullptr;
				}
				OnDisconnect();
				_pReadKeepAlive = nullptr;
			}
			else if( !headerLength )
				ERR( "Failed no length"sv );
			else
				ReadBody( MessageLength(_readMessageSize) );
		});
	}

	α ProtoClientSession::ReadBody( int messageLength )ι->void{
		google::protobuf::uint8 buffer[4096];
		up<google::protobuf::uint8[]> pData;
		var useHeap = messageLength>sizeof(buffer);
		if( useHeap )
			pData = up<google::protobuf::uint8[]>{ new google::protobuf::uint8[messageLength] };
		auto pBuffer = useHeap ? pData.get() : buffer;
		boost::system::error_code ec;
		std::size_t length = net::read( _socket, net::buffer(reinterpret_cast<void*>(pBuffer), messageLength), ec );
		if( ec || length!=messageLength ){
			ERR_IF( length!=messageLength, "'{}' read!='{}' expected", length, messageLength );
			else
				ERR( "Read Body Failed - {}"sv, ec.value() );
			_socket.close();
		}
		else{
			Process( pBuffer, messageLength );
			ReadHeader();
		}
	}
	#pragma clang diagnostic ignored "-Wunused-lambda-capture"
	α ProtoClientSession::Write( up<google::protobuf::uint8[]> p, uint c )ι->void{
		auto b = net::buffer( p.get(), c );
		net::async_write( _socket, b, [this, _=move(p), c, b2=move(b)]( std::error_code ec, uint length ){
			if( ec ){
				auto keepAlive = _pIOContext;
				LOGX( "({})Write message returned '{}'.", ec.value(), ec.message() );
				if( _pIOContext ){
					if(  _socket.is_open() )
						_socket.close();
					_pIOContext = nullptr;
				}
				_pWriteKeepAlive = nullptr;
			}
			else if( c!=length )
				ERRX( "bufferSize!=length, {}!={}", c, length );
		});
	}
/////////////////////////////////////////////////////////////////////////////////////
	ProtoClient::ProtoClient( str settingsPath, PortType defaultPort )ε:
		IClientSocket{ settingsPath, defaultPort }{
		Connect();
	}

	α ProtoClient::Connect()ε->void{
		if( !_pIOContext )
			_pIOContext = AsioContextThread::Instance();

		tcp::resolver resolver( _pIOContext->Context() );
		auto endpoints = resolver.resolve( Host, std::to_string(Port).c_str() );
		try{
			net::connect( _socket, endpoints );
			ReadHeader();
		}
		catch( const boost::system::system_error& e ){
			throw NetException{ format("{}:{}", Host, Port), {}, (uint)e.code().value(), string{e.what()}, Jde::ELogLevel::Debug };
		}
	}
}