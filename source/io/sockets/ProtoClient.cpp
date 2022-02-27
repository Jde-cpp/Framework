#include "ProtoClient.h"
#include "../../threading/Thread.h"
#include "../../threading/InterruptibleThread.h"
#define var const auto

namespace Jde::IO::Sockets
{
#define _logLevel Sockets::LogLevel()
	ProtoClientSession::ProtoClientSession( /*boost::asio::io_context& context*/ ):
		_pIOContext{ IOContextThread::Instance() },
		_socket{ _pIOContext->Context() }
	{}
	uint32 ProtoClientSession::MessageLength( char* readMessageSize )noexcept
	{
		uint32_t length;
		char* pDestination = reinterpret_cast<char*>( &length );
		const char* pStart = readMessageSize;
		std::copy( pStart, pStart+4, pDestination );
		return ntohl( length );
	}
	void ProtoClientSession::ReadHeader()noexcept
	{
		net::async_read( _socket, net::buffer(static_cast<void*>(_readMessageSize), sizeof(_readMessageSize)), [this]( std::error_code ec, std::size_t headerLength )
		{
			if( ec )
			{
				if( ec.value()==2 )
					LOG( "ProtoClientSession::ReadHeader closing - 2 '{}'", CodeException::ToString(ec) );
				else if( ec.value()==125 )
					LOG( "_socket.close() ec='{}'", CodeException::ToString(ec) );
				else
					DBG( "Client::ReadHeader Failed - ({}){} closing"sv, ec.value(), ec.message() );
			}
			else if( !headerLength )
				ERR( "Failed no length"sv );
			else
			{
				var messageLength = MessageLength( _readMessageSize );
				ReadBody( messageLength );
			}
		});
	}

	α ProtoClientSession::ReadBody( int messageLength )noexcept->void
	{
		google::protobuf::uint8 buffer[4096];
		up<google::protobuf::uint8[]> pData;
		var useHeap = messageLength>sizeof(buffer);
		if( useHeap )
			pData = up<google::protobuf::uint8[]>{ new google::protobuf::uint8[messageLength] };
		auto pBuffer = useHeap ? pData.get() : buffer;
		boost::system::error_code ec;
		std::size_t length = net::read( _socket, net::buffer(reinterpret_cast<void*>(pBuffer), messageLength), ec );
		if( ec || length!=messageLength )
		{
			ERR_IF( length!=messageLength, "'{}' read!='{}' expected", length, messageLength );
			else
				ERR( "Read Body Failed - {}"sv, ec.value() );
			_socket.close();
		}
		else
		{
			Process( pBuffer, messageLength );
			ReadHeader();
		}
	}
	α ProtoClientSession::Write( up<google::protobuf::uint8[]> p, uint c )noexcept->void
	{
		auto b = net::buffer( p.get(), c );
		net::async_write( _socket, b, [this, _=move(p), c, b2=move(b)]( std::error_code ec, uint length )
		{
			if( ec )
			{
				LOGX( "({})Write message returned '{}'.", ec.value(), ec.message() );
				_socket.close();
				_pIOContext = nullptr;
			}
			else if( c!=length )
				ERRX( "bufferSize!=length, {}!={}", c, length );
		});
	}
/////////////////////////////////////////////////////////////////////////////////////
	ProtoClient::ProtoClient( str settingsPath, PortType defaultPort )noexcept(false):
		IClientSocket{ settingsPath, defaultPort }
	{
		Connect();
	}

	α ProtoClient::Connect()noexcept(false)->void
	{
		if( !_pIOContext )
			_pIOContext = IOContextThread::Instance();

		tcp::resolver resolver( _pIOContext->Context() );
		auto endpoints = resolver.resolve( Host, std::to_string(Port).c_str() );
		try
		{
			net::connect( _socket, endpoints );
			ReadHeader();
		}
		catch( const boost::system::system_error& e )
		{
			throw NetException{ format("{}:{}", Host, Port), {}, (uint)e.code().value(), string{e.what()}, Jde::ELogLevel::Warning };
		}
	}
}