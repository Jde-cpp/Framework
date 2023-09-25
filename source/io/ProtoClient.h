#pragma once
#include <boost/asio.hpp>
#include <jde/Log.h>
#include <jde/Exception.h>
#include "../collections/Queue.h"
#include "AsioContextThread.h"
#include "ProtoUtilities.h"
#include "Socket.h"

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
		void Close( std::condition_variable* pCvClient=nullptr )ι;
		virtual void OnConnected()ι{};
		static uint32 MessageLength( char* readMessageSize )ι;
	protected:
		virtual void OnDisconnect()ι=0;
		α ReadHeader()ι->void;
		α ReadBody( int messageLength )ι->void;
		α Write( up<google::protobuf::uint8[]> p, uint c )ι->void;
		virtual void Process( google::protobuf::uint8* pData, int size )ι=0;

		sp<AsioContextThread> _pIOContext;
		tcp::socket _socket;
		char _readMessageSize[4];
	};
#pragma warning(push)
#pragma warning( disable : 4459 )
	struct ProtoClient : IClientSocket, ProtoClientSession
	{
		ProtoClient( str settingsPath, PortType defaultPort )ε;
		virtual ~ProtoClient()=0;
		void Connect()ε;
	};
	inline ProtoClient::~ProtoClient(){}
#pragma warning(pop)
#define $ template<typename TOut, typename TIn> auto TProtoClient<TOut,TIn>
	template<typename TOut, typename TIn>
	struct TProtoClient : ProtoClient
	{
		TProtoClient( str settingsPath, PortType defaultPort )ε:
			ProtoClient{ settingsPath, defaultPort }
		{}

   	virtual ~TProtoClient()=default;
		void Process( google::protobuf::uint8* pData, int size )ι override;
		void Write( TOut&& message )ι;
		virtual void OnReceive( TIn& pIn )ι=0;
	};
#define var const auto
	$::Process( google::protobuf::uint8* pData, int size )ι->void
	{
		try
		{
			auto transmission = Proto::Deserialize<TIn>( pData, size );
			OnReceive( transmission );
		}
		catch( const IException& )
		{}
	}

	$::Write( TOut&& m )ι->void
	{
		auto [p,size] = IO::Proto::SizePrefixed( move(m) );
		ProtoClientSession::Write( move(p), size );
	}
}
#undef var
#undef $