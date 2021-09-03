#include "ProtoClient.h"
#include "../../threading/Thread.h"
#include "../../threading/InterruptibleThread.h"
#define var const auto

namespace Jde::IO::Sockets
{
	ProtoClientSession::ProtoClientSession( boost::asio::io_context& context ):
		//_pSocket{ make_unique<basio::ip::tcp::socket>(context) }
		_socket{ context }
	{

	}
	/*VectorPtr<google::protobuf::uint8> ProtoClientSession::ToBuffer( const google::protobuf::MessageLite& message )noexcept(false)
	{
		const uint32_t length = (uint32_t)message.ByteSizeLong();
		auto pData = make_shared<vector<google::protobuf::uint8>>( length+4 );
		auto pDestination = pData->data();
		const char* pLength = reinterpret_cast<const char*>( &length )+3;
		for( auto i=0; i<4; ++i )
			*pDestination++ = *pLength--;
		var result = message.SerializeToArray( pData->data()+4, (int)pData->size()-4 );
		if( !result )
			THROW( Exception("Could not serialize to an array") );
		//DBGX( "result={}", result );
		return pData;
	}
*/
/*	void ProtoClientSession::Disconnect()noexcept
	{
		if( _pSocket )
		{
			//if( _pSocket->get_executor() )
			//	_pSocket->close();//_service==null throws...
			//delete _pSocket;
			_pSocket = nullptr;
		}
		//OnDisconnect(); called in destructor
	}*/
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
		basio::async_read( _socket, basio::buffer(static_cast<void*>(_readMessageSize), sizeof(_readMessageSize)), [this]( std::error_code ec, std::size_t headerLength )
		{
			if( ec )
			{
				_connected = false;
				if( ec.value()==125 )
					LOG( _logLevel, "Client::ReadHeader closing - '{}'", CodeException::ToString(ec) );
				else
					ERR( "Client::ReadHeader Failed - '{}' closing"sv, ec.value() );
			}
			else if( !headerLength )
				ERR( "Failed no length"sv );
			else
			{
				var messageLength = MessageLength( _readMessageSize );
				LOG( _logLevel, "bodyLength='{:L}'", messageLength );
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
		std::size_t length = basio::read( _socket, basio::buffer(reinterpret_cast<void*>(pBuffer), messageLength), ec );
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
	ProtoClient::ProtoClient( sv clientThreadName, str settingsPath, PortType defaultPort )noexcept(false):
		PerpetualAsyncSocket{ clientThreadName, settingsPath, defaultPort },
		ProtoClientSession{ _asyncHelper }
	{
		Connect();
	}

	void ProtoClient::Connect()noexcept(false)
	{
		_initialized = true;
		basio::ip::tcp::resolver resolver( _asyncHelper );
		auto endpoints = resolver.resolve( Host, std::to_string(Port).c_str() );
		try
		{
			basio::connect( _socket, endpoints );
			INFO( "({}) connected to '{}:{}' "sv, ClientThreadName, Host, Port );
			_connected = true;
			ReadHeader();
		}
		catch( const boost::system::system_error& e )
		{
			THROW( "Could not connect {}", e.what() );
		}
	}
}