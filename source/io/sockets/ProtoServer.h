#pragma once
#include "../../Exports.h"
//#include "Socket.h"
#include "ProtoClient.h"
#include "google/protobuf/message.h"
#include "../../collections/Map.h"

#define var const auto

namespace Jde::IO::Sockets
{
	typedef uint32 SessionPK;
	struct ProtoSession;

	//template<typename T>
	//struct ISession
	//{
	//	virtual void OnReceive( sp<T> pValue )noexcept(false)=0;
	//};

	struct ProtoServer : public AsyncSocket
	{
		JDE_NATIVE_VISIBILITY ProtoServer( PortType port )noexcept(false);
		JDE_NATIVE_VISIBILITY virtual ~ProtoServer();
		JDE_NATIVE_VISIBILITY void Close()noexcept;
		virtual sp<ProtoSession> CreateSession( basio::ip::tcp::socket socket, SessionPK id )noexcept=0;
		void RemoveSession( SessionPK id )noexcept{ _sessions.erase(id); }
	protected:
		JDE_NATIVE_VISIBILITY void Accept()noexcept;
		std::atomic<SessionPK> _id{0};
		unique_ptr<basio::ip::tcp::acceptor> _pAcceptor;
		Map<SessionPK,ProtoSession> _sessions;
	private:
		void Run()noexcept;
	};

	//template<typename T>
/*	struct TProtoServer : public ProtoServer
	{
		TProtoServer( PortType port )noexcept(false):
			ProtoServer{port}
		{
			Accept();
		}
		//virtual std::shared_ptr<ISession<T>> OnSessionStart( SessionPK id )noexcept=0;
	protected:
	};
	*/
	struct JDE_NATIVE_VISIBILITY ProtoSession//: public Session, public std::enable_shared_from_this<ServerSession>
	{
		ProtoSession( basio::ip::tcp::socket& socket, SessionPK id )noexcept;
		SessionPK Id;
		//virtual void Start()noexcept=0;
		virtual void OnDisconnect()noexcept=0;
	protected:		
		void ReadHeader()noexcept;
		basio::ip::tcp::socket _socket2;
	private:
		virtual void ReadBody( uint messageLength )noexcept=0;
		char _readMessageSize[4];
	};

#pragma region TProtoSession
	template<typename TToServer, typename TFromServer>
	struct TProtoSession: public ProtoSession//, public std::enable_shared_from_this<ServerSession>
	{
		TProtoSession( basio::ip::tcp::socket& socket, SessionPK id )noexcept:
			ProtoSession{ socket, id }
		{}
		
	protected:
		//void Write( sp<TFromServer> pMessage )noexcept;
		virtual void OnReceive( sp<TToServer> pValue )noexcept(false)=0;
		void ReadBody( uint messageLength )noexcept override;
		void Write( const TFromServer& message )noexcept;
		vector<google::protobuf::uint8> _message;
	};
	
	template<typename TToServer, typename TFromServer>
	void TProtoSession<TToServer,TFromServer>::ReadBody( uint messageLength )noexcept
	{
		auto onDone = [&, messageLength]( std::error_code ec, std::size_t length )
		{
			try
			{
				if( ec )
					THROW( IOException("Read Body Failed - '{}'", ec.value()) );
				if( length!=messageLength )
					THROW( IOException("Read Body read '{}' expected '{}'", length, messageLength) );

				TRACE( "ServerSession::ReadBody '{}' bytes", length );
				google::protobuf::io::CodedInputStream input( _message.data(), (int)std::min(length,_message.size()) );
				auto pTransmission = make_shared<TToServer>(); 
				if( !pTransmission->MergePartialFromCodedStream(&input) )
					THROW( IOException("MergePartialFromCodedStream returned false, length={}", length) );
				OnReceive( pTransmission );
			}
			catch( const IOException& e )
			{
				ERR( "ReadBody failed, TODO return {}", e.what() );
			}
			ReadHeader();
		};
		if( _message.size()<messageLength )
			_message.resize( messageLength );
		void* pData = static_cast<void*>( _message.data() );
		basio::async_read( _socket2, basio::buffer(pData, messageLength), onDone );
	}

	template<typename TToServer, typename TFromServer>
	void TProtoSession<TToServer,TFromServer>::Write( const TFromServer& message )noexcept
	{
		auto pData = ProtoClientSession::ToBuffer( message );
		auto onDone = [pData]( std::error_code ec, std::size_t length )
		{
			if( ec )
				ERR( "Write message returned '{}'.", ec.value() );
			else
				TRACE( "Session::Write length:  '{}'.", length );
		};
		basio::async_write( _socket2, basio::buffer( pData->data(), pData->size() ), onDone );
	}

#pragma endregion
	//template<typename T>
	
}