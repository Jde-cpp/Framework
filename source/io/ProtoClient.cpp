#include "ProtoClient.h"
#include "../threading/Thread.h"
#include "../threading/InterruptibleThread.h"
#define var const auto

namespace Jde::IO::Sockets{

//#define _logLevel Sockets::LogLevel()
	sp<LogTag> _logLevel{ Logging::TagLevel( "net" ) };

	ProtoClientSession::ProtoClientSession( /*boost::asio::io_context& context*/ ):
		_pIOContext{ AsioContextThread::Instance() },
		_socket{ _pIOContext->Context() }
	{}

	α ProtoClientSession::Close( std::condition_variable* pCvClient )ι->void{
		DBG( "ProtoClientSession::Close is_open={}"sv, _socket.is_open() );
		if( _pIOContext && _socket.is_open() )
			DBG( "ProtoClientSession::Close IOContext use_count={}"sv, _pIOContext.use_count() );
			_socket.close();
		//_pIOContext = nullptr;
		if( pCvClient )
			pCvClient->notify_one();
	}

	α ProtoClientSession::MessageLength( const char* readMessageSize )ι->uint32{
		uint32_t length;
		char* pDestination = reinterpret_cast<char*>( &length );
		const char* pStart = readMessageSize;
		std::copy( pStart, pStart+4, pDestination );
		return ntohl( length );
	}

	α ProtoClientSession::ReadHeader()ι->void{
		auto pReadKeepAlive = shared_from_this();
		net::async_read( _socket, net::buffer(static_cast<void*>(_readMessageSize), sizeof(_readMessageSize)), [pThis=move(pReadKeepAlive)]( std::error_code ec, std::size_t headerLength )
		{
			if( ec ){
				if( ec.value()==2 )
					DBG( "ProtoClientSession::ReadHeader closing - 2 '{}'", CodeException::ToString(ec) );
				else if( ec.value()==125 )
					DBG( "_socket.close() ec='{}'", CodeException::ToString(ec) );
				else
					DBG( "Client::ReadHeader Failed - ({}){} closing"sv, ec.value(), ec.message() );
//				auto keepAlive = _pIOContext;
				pThis->Close();
				pThis->OnDisconnect();
			}
			else if( !headerLength )
				ERR( "Failed no length"sv );
			else
				pThis->ReadBody( MessageLength(pThis->_readMessageSize) );
//			_pReadKeepAlive = nullptr;
		});
	}

	α ProtoClientSession::ReadBody( int messageLength )ι->void{
		google::protobuf::uint8 buffer[4096];
		up<google::protobuf::uint8[]> pData;
		var useHeap = messageLength>sizeof(buffer);
		if( useHeap )
			pData = std::make_unique<google::protobuf::uint8[]>( messageLength );
		auto pBuffer = useHeap ? pData.get() : buffer;
		boost::system::error_code ec;
		std::size_t length = net::read( _socket, net::buffer(reinterpret_cast<void*>(pBuffer), messageLength), ec );
		if( ec || length!=messageLength ){
			ERR_IF( length!=messageLength, "'{}' read!='{}' expected", length, messageLength );
			else
				ERR( "Read Body Failed - {}"sv, ec.value() );
			Close();
		}
		else{
			Process( pBuffer, messageLength );
			ReadHeader();
		}
	}
	#pragma clang diagnostic ignored "-Wunused-lambda-capture"
	α ProtoClientSession::Write( up<google::protobuf::uint8[]> p, uint c )ι->void{
		auto b = net::buffer( p.get(), c );
		auto pKeepAlive = shared_from_this();
		net::async_write( _socket, b, [this, thisKeepAlive=move(pKeepAlive), _=move(p), c, b2=move(b)]( std::error_code ec, uint length ){
			if( ec ){
//				auto keepAlive = _pIOContext;
				LOGX( "Write message returned '({:x}){}'.", ec.value(), ec.message() );
				Close();
				//if( _pIOContext ){
				//	if(  _socket.is_open() )
				//		_socket.close();
				//	_pIOContext = nullptr;
				//}
			}
			else if( c!=length )
				ERRX( "bufferSize!=length, {}!={}", c, length );
		});
	}
/////////////////////////////////////////////////////////////////////////////////////
	ProtoClient::ProtoClient( str settingsPath, PortType defaultPort )ε:
		IClientSocket{ settingsPath, defaultPort }
	{}

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