﻿#pragma once
#ifndef PROTO_CLIENT_H
#define PROTO_CLIENT_H
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
	namespace net = boost::asio;
	using tcp = net::ip::tcp;

	struct Γ ProtoClientSession : std::enable_shared_from_this<ProtoClientSession>{
		ProtoClientSession();
		virtual ~ProtoClientSession(){ TRACET( LogTag(), "~ProtoClientSession -start" ); _socket.close(); TRACET( LogTag(), "~ProtoClientSession-end" ); };
		α Close( std::condition_variable* pCvClient=nullptr )ι->void;
		β OnConnected()ι->void{};
		Ω MessageLength( const char* readMessageSize )ι->uint32;
	protected:
		β OnDisconnect()ι->void=0;
		α ReadHeader()ι->void;
		α ReadBody( uint32 messageLength )ι->void;
		α Write( up<google::protobuf::uint8[]> p, uint c )ι->void;
		β Process( google::protobuf::uint8* pData, int size )ι->void=0;
		α LogTag()ι->sp<Jde::LogTag>;
		sp<ProtoClientSession> _pReadKeepAlive; sp<ProtoClientSession> _pWriteKeepAlive;
		sp<AsioContextThread> _pIOContext;
		tcp::socket _socket;
		char _readMessageSize[4];
	};
#pragma warning(push)
#pragma warning( disable : 4459 )
	struct ProtoClient : IClientSocket, ProtoClientSession{
		ProtoClient( str settingsPath, PortType defaultPort )ε;
		virtual ~ProtoClient()=0;
		α Connect()ε->void;
	};
	inline ProtoClient::~ProtoClient(){}
#pragma warning(pop)
#define $ template<typename TOut, typename TIn> auto TProtoClient<TOut,TIn>
	template<typename TOut, typename TIn>
	struct TProtoClient : ProtoClient{
		TProtoClient( str settingsPath, PortType defaultPort )ε:
			ProtoClient{ settingsPath, defaultPort }
		{}

   	virtual ~TProtoClient()=default;
		α Process( google::protobuf::uint8* pData, int size )ι->void override;
		α Write( TOut&& message )ι->void;
		β OnReceive( TIn& pIn )ι->void=0;
	};
#define var const auto
	$::Process( google::protobuf::uint8* pData, int size )ι->void{
		try{
			auto transmission = Proto::Deserialize<TIn>( pData, size );
			OnReceive( transmission );
		}
		catch( const IException& )
		{}
	}

	$::Write( TOut&& m )ι->void{
		auto [p,size] = IO::Proto::SizePrefixed( move(m) );
		ProtoClientSession::Write( move(p), size );
	}
}
#undef var
#undef $
#endif