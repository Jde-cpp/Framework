#pragma once
#include "Socket.h"
#include "../../collections/Queue.h"
#include "../ProtoUtilities.h"
#include <jde/Log.h>
#include <jde/Exception.h>

DISABLE_WARNINGS
#include <google/protobuf/message_lite.h>
ENABLE_WARNINGS

namespace Jde::IO::Sockets
{
	namespace net = boost::asio;
	using tcp = net::ip::tcp;

	struct ProtoClientSession
	{
		ProtoClientSession( /*boost::asio::io_context& context*/ );
		virtual ~ProtoClientSession(){ DBGX( "~{}"sv, "ProtoClientSession" ); };
		void Close( std::condition_variable* pCvClient=nullptr )noexcept;
		virtual void OnClose()noexcept{};
		virtual void OnConnected()noexcept{};
		static uint32 MessageLength( char* readMessageSize )noexcept;
	protected:
		virtual void OnDisconnect()noexcept=0;
		void ReadHeader()noexcept;
		void ReadBody( uint messageLength )noexcept;
		virtual void Process( google::protobuf::uint8* pData, int size )noexcept=0;

		std::atomic<bool> _connected{false};
		tcp::socket _socket;
		char _readMessageSize[4];
	};
#pragma warning(push)
#pragma warning( disable : 4459 )
	struct ProtoClient : IClientSocket, ProtoClientSession
	{
		ProtoClient( sv clientThreadName, str settingsPath, PortType defaultPort )noexcept(false);
		virtual ~ProtoClient()=0;
		void Connect()noexcept(false);
	//private:
		//void Run( sv name )noexcept;
	};
	inline ProtoClient::~ProtoClient(){}
#pragma warning(pop)
#define $ template<typename TOut, typename TIn> auto TProtoClient<TOut,TIn>
	template<typename TOut, typename TIn>
	struct TProtoClient : ProtoClient
	{
		TProtoClient( sv clientThreadName, str settingsPath, PortType defaultPort )noexcept(false):
			ProtoClient{ clientThreadName, settingsPath, defaultPort }
		{}

   	virtual ~TProtoClient()=default;
		void Process( google::protobuf::uint8* pData, int size )noexcept override;
		void Write( const TOut& message )noexcept;
		virtual void OnReceive( TIn& pIn )noexcept=0;
	//private:
//		void Write( tuple<unique_ptr<google::protobuf::uint8[]>,uint>&& s )noexcept(false);
	};
#define var const auto
	$::Process( google::protobuf::uint8* pData, int size )noexcept->void
	{
		try
		{
			auto transmission = Proto::Deserialize<TIn>( pData, size );
			OnReceive( transmission );
		}
		catch( const Exception& e )
		{}
	}

	$::Write( const TOut& msg )noexcept->void
	{
		auto data = IO::Proto::SizePrefixed( msg );
		auto pBuffer = move( get<0>(data) ); var bufferSize = get<1>( data );
		net::async_write( _socket, net::buffer(pBuffer.get(), bufferSize), [this, _=move(pBuffer), bufferSize]( std::error_code ec, uint length )
		{
			ASSERT( bufferSize==length );
			if( ec )
			{
				ERR( "async_write Failed - {}"sv, ec.value() );
				_socket.close();
			}
		});
	}

/*	$::Write( tuple<unique_ptr<google::protobuf::uint8[]>,uint>&& data )noexcept(false)->void
	{
		//TRACEX( "Writing:  {}"sv, s.size() );
		auto pBuffer = move( get<0>(data) );
		net::async_write( _socket, net::buffer(pBuffer, get<1>(data.get())), [this, _=move(pBuffer)](std::error_code ec, std::size_t / *length* /)
		{
			if( ec )
			{
				ERR( "async_write Failed - {}"sv, ec.value() );
				_socket.close();
			}
		});
	}
	*/
#undef var
#undef $
}