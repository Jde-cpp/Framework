#include "ProtoClient.h"
#include "../../threading/Thread.h"
#include "../../threading/InterruptibleThread.h"
#define var const auto

namespace Jde::IO::Sockets
{
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
					LOG( LogLevel(), "ProtoClientSession::ReadHeader closing - 2 '{}'", CodeException::ToString(ec) );
				else if( ec.value()==125 )
					LOG( LogLevel(), "_socket.close() ec='{}'", CodeException::ToString(ec) );
				else
					ERR( "Client::ReadHeader Failed - '{}' closing"sv, ec.value() );
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

	void ProtoClientSession::ReadBody( uint messageLength )noexcept
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
/////////////////////////////////////////////////////////////////////////////////////
	ProtoClient::ProtoClient( str settingsPath, PortType defaultPort )noexcept(false):
		IClientSocket{ settingsPath, defaultPort }
	{
		Connect();
	}

	void ProtoClient::Connect()noexcept(false)
	{
		if( !_pIOContext )
			_pIOContext = IOContextThread::Instance();

		tcp::resolver resolver( _pIOContext->Context() );
		auto endpoints = resolver.resolve( Host, std::to_string(Port).c_str() );
		try
		{
			net::connect( _socket, endpoints );
			_connected = true;
			ReadHeader();
		}
		catch( const boost::system::system_error& e )
		{
			THROW( "Could not connect {}", e.what() );
		}
	}
}