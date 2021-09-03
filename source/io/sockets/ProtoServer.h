#pragma once
#include <jde/Exports.h>
//#include "Socket.h"
#include "ProtoClient.h"
#include "google/protobuf/message.h"
#include "../ProtoUtilities.h"
#include "../../collections/Map.h"

#define var const auto

namespace Jde::IO::Sockets
{
	typedef uint32 SessionPK;
	struct ProtoSession;

	#define 🚪 JDE_NATIVE_VISIBILITY auto
	struct ProtoServer : public PerpetualAsyncSocket
	{
		JDE_NATIVE_VISIBILITY ProtoServer( sv clientThreadName, str settingsPath, PortType defaultPort )noexcept(false);
		JDE_NATIVE_VISIBILITY virtual ~ProtoServer();
		//🚪 Close()noexcept->void;
		virtual up<ProtoSession> CreateSession( basio::ip::tcp::socket&& socket, SessionPK id )noexcept=0;
		void RemoveSession( SessionPK id )noexcept{ unique_lock l{_mutex}; _sessions.erase(id); }

	protected:
		🚪 Accept()noexcept->void;
		std::atomic<SessionPK> _id{0};
		basio::ip::tcp::acceptor _acceptor;
		flat_map<SessionPK,up<ProtoSession>> _sessions; std::shared_mutex _mutex;
	private:
		void Run()noexcept;
	};

	struct JDE_NATIVE_VISIBILITY ProtoSession
	{
		ProtoSession( basio::ip::tcp::socket&& socket, SessionPK id )noexcept;
		virtual ~ProtoSession()=default;
		SessionPK Id;
		//virtual void Start()noexcept=0;
		virtual void OnDisconnect()noexcept=0;
	protected:
		void ReadHeader()noexcept;
		basio::ip::tcp::socket _socket;
		ELogLevel _logLevel{ ELogLevel::Debug };
	private:
		virtual void ReadBody( uint messageLength )noexcept=0;
		char _readMessageSize[4];
	};

#pragma region TProtoSession
#define $ template<class TToServer, class TFromServer> auto TProtoSession<TToServer,TFromServer>::
	template<class TToServer, class TFromServer>
	struct TProtoSession: public ProtoSession
	{
		TProtoSession( basio::ip::tcp::socket&& socket, SessionPK id )noexcept:
			ProtoSession{ move(socket), id }
		{}

	protected:
		virtual void OnReceive( TToServer&& pValue )noexcept(false)=0;
		void ReadBody( uint messageLength )noexcept override;
		void Write( const TFromServer& message )noexcept;
		vector<google::protobuf::uint8> _message;
	};

//TODO consolidate with ProtoClientSession::ReadBody
	$ ReadBody( uint messageLength )noexcept->void
	{
		google::protobuf::uint8 buffer[4096];
		var useHeap = messageLength>sizeof(buffer);
		var pData = useHeap ? up<google::protobuf::uint8[]>{ new google::protobuf::uint8[messageLength] } : up<google::protobuf::uint8[]>{};
		auto pBuffer = useHeap ? pData.get() : buffer;
		try
		{
			var length = basio::read( _socket, basio::buffer(reinterpret_cast<void*>(pBuffer), messageLength) ); THROW_IF( length!=messageLength, "'{}' read!='{}' expected", length, messageLength );
			OnReceive( Proto::Deserialize<TToServer>(pBuffer, length) );
			ReadHeader();
		}
		catch( boost::system::system_error& e )
		{
			ERR( "Read Body Failed - {}"sv, e.what() );
			_socket.close();
		}
		catch( const Exception& )
		{}
	}

	$ Write( const TFromServer& value )noexcept->void
	{
		auto [p,size] = IO::Proto::SizePrefixed( value );
		basio::async_write( _socket, basio::buffer(p.get(), size), [x=move(p)]( std::error_code ec, std::size_t length )
		{
			if( ec )
				DBG( "({})Write message returned '{}'."sv, 5, ec.value() );
			else
				TRACE( "Session::Write length:  '{}'."sv, length );
		});
	}

#pragma endregion
#undef 🚪
#undef $
}