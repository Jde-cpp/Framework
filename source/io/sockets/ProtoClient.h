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
	static const LogTag& _logLevel = Logging::TagLevel( "net" );
	namespace net = boost::asio;
	using tcp = net::ip::tcp;

	struct ProtoClientSession
	{
		ProtoClientSession();
		virtual ~ProtoClientSession(){ LOGX( "~ProtoClientSession -start"); _socket.close(); LOGX( "~ProtoClientSession-end"); };
		void Close( std::condition_variable* pCvClient=nullptr )noexcept;
		virtual void OnConnected()noexcept{};
		static uint32 MessageLength( char* readMessageSize )noexcept;
	protected:
		virtual void OnDisconnect()noexcept=0;
		α ReadHeader()noexcept->void;
		α ReadBody( int messageLength )noexcept->void;
		α Write( up<google::protobuf::uint8[]> p, uint c )noexcept->void;
		virtual void Process( google::protobuf::uint8* pData, int size )noexcept=0;

		sp<IOContextThread> _pIOContext;
		tcp::socket _socket;
		char _readMessageSize[4];
	};
#pragma warning(push)
#pragma warning( disable : 4459 )
	struct ProtoClient : IClientSocket, ProtoClientSession
	{
		ProtoClient( str settingsPath, PortType defaultPort )noexcept(false);
		virtual ~ProtoClient()=0;
		void Connect()noexcept(false);
	};
	inline ProtoClient::~ProtoClient(){}
#pragma warning(pop)
#define $ template<typename TOut, typename TIn> auto TProtoClient<TOut,TIn>
	template<typename TOut, typename TIn>
	struct TProtoClient : ProtoClient
	{
		TProtoClient( str settingsPath, PortType defaultPort )noexcept(false):
			ProtoClient{ settingsPath, defaultPort }
		{}

   	virtual ~TProtoClient()=default;
		void Process( google::protobuf::uint8* pData, int size )noexcept override;
		void Write( const TOut& message )noexcept;
		virtual void OnReceive( TIn& pIn )noexcept=0;
	};
#define var const auto
	$::Process( google::protobuf::uint8* pData, int size )noexcept->void
	{
		try
		{
			auto transmission = Proto::Deserialize<TIn>( pData, size );
			OnReceive( transmission );
		}
		catch( const IException& )
		{}
	}

	$::Write( const TOut& m )noexcept->void
	{
		auto [p,size] = IO::Proto::SizePrefixed( m );
		ProtoClientSession::Write( move(p), size );
	}
}
#undef var
#undef $