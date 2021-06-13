#include "ProtoClient.h"
#include "../../threading/Thread.h"
#include "../../threading/InterruptibleThread.h"
#define var const auto

namespace Jde::IO::Sockets
{
	ProtoClientSession::ProtoClientSession( boost::asio::io_context& context ):
		//_pSocket{ make_unique<basio::ip::tcp::socket>(context) }
		_pSocket{ new basio::ip::tcp::socket{context} }
	{
	}
	VectorPtr<google::protobuf::uint8> ProtoClientSession::ToBuffer( const google::protobuf::MessageLite& message )noexcept(false)
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
	void ProtoClientSession::Disconnect()noexcept
	{
		if( _pSocket )
		{
			//if( _pSocket->get_executor() )
			//	_pSocket->close();//_service==null throws...
			//delete _pSocket;
			_pSocket = nullptr;
		}
		//OnDisconnect(); called in destructor
	}
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
		auto onComplete = [this]( std::error_code ec, std::size_t headerLength )
		{
			if( ec )
			{
				_connected = false;
				if( ec.value()==125 )
					TRACE( "Client::ReadHeader closing - '{}'"sv, CodeException::ToString(ec) );
				else
					ERR( "Client::ReadHeader Failed - '{}' closing"sv, ec.value() );
					//ERR( "Client::ReadHeader Failed - '{}' closing", CodeException::ToString(ec) );crashes for some reason.
				Disconnect();
			}
			else if( !headerLength )
				ERR( "Failed no length"sv );
			else
			{
				var messageLength = MessageLength( _readMessageSize );
				TRACE( "'{}' bytes"sv, messageLength );
				ReadBody( messageLength );
			}
		};
		basio::async_read( *_pSocket, basio::buffer(static_cast<void*>(_readMessageSize), sizeof(_readMessageSize)), onComplete );
	}

	void ProtoClientSession::ReadBody( uint messageLength )noexcept
	{
		auto onRead = [this, messageLength]( std::error_code ec, std::size_t length )
		{
			if( ec )
			{
				ERR( "Read Body Failed - {}"sv, ec.value() );
				_pSocket->close();
				//delete _pSocket;
				_pSocket = nullptr;
			}
			else
			{
				if( length==messageLength )
					TRACE( "'{}' bytes read==expecting"sv, length );
				else
					ERR( "'{}' read!='{}' expected"sv, length, messageLength );
				Process( _message.data(), std::min<size_t>(length,messageLength) );
				ReadHeader();
			}
		};
		if( _message.size()<messageLength )
			_message.resize( messageLength, 0 );
		basio::async_read( *_pSocket, basio::buffer(reinterpret_cast<void*>(_message.data()), messageLength), onRead );
	}
/////////////////////////////////////////////////////////////////////////////////////
	ProtoClient::ProtoClient( sv host, uint16 port, sv name )noexcept:
		ProtoClientSession{ _asyncHelper },
		Name{ name },
		_host{host},
		_port{port}
	{}

	void ProtoClient::Startup( sv clientThreadName )noexcept
	{
		Connect();
		RunAsyncHelper( clientThreadName );
	}
	void ProtoClient::Connect()noexcept
	{
		basio::ip::tcp::resolver resolver( _asyncHelper );
		auto endpoints = resolver.resolve( _host, std::to_string(_port).c_str() );
		try
		{
			Connect( endpoints );
		}
		catch( const Exception& /*e*/ )
		{}
	}
	void ProtoClient::Connect( const basio::ip::tcp::resolver::results_type& endpoints )noexcept(false)
	{
		auto onConnect = [this](std::error_code ec, basio::ip::tcp::endpoint)
		{
			if( ec )
				ERR( "Connect Failed - {}"sv, ec.value() );
			else
			{
				_connected = true;
				ReadHeader();
			}
		};
		//basio::async_connect( _socket, endpoints, onConnect );
		try
		{
			if( !_pSocket )
				_pSocket = make_unique<basio::ip::tcp::socket>( _asyncHelper );//_pSocket = make_unique<basio::ip::tcp::socket>( _asyncHelper );
			auto result = basio::connect( *_pSocket, endpoints );
			TRACE( "Client::Connect"sv );
			onConnect( std::error_code(), result );
		}
		catch( const boost::system::system_error& e )
		{
			//ERR0( e.what() );
			THROW( Exception( "Could not connect {}", e.what()) );
		}
	}
}