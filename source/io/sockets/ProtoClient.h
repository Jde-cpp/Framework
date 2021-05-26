#pragma once
#include "Socket.h"
#include "../../collections/Queue.h"
#include <jde/Log.h>
#include <jde/Exception.h>

DISABLE_WARNINGS
#include <google/protobuf/message_lite.h>
ENABLE_WARNINGS

namespace Jde::IO::Sockets
{
	namespace basio = boost::asio;
	struct ProtoClientSession
	{
		ProtoClientSession( boost::asio::io_context& context );
		virtual ~ProtoClientSession(){ DBGX( "~{}"sv, "ProtoClientSession" ); _pSocket=nullptr; };
		void Close( std::condition_variable* pCvClient=nullptr )noexcept;
		virtual void OnClose()noexcept{};
		virtual void OnConnected()noexcept{};
		JDE_NATIVE_VISIBILITY static VectorPtr<google::protobuf::uint8> ToBuffer( const google::protobuf::MessageLite& msg )noexcept(false);
		static uint32 MessageLength( char* readMessageSize )noexcept;
	protected:
		void Disconnect()noexcept;
		virtual void OnDisconnect()noexcept=0;
		void ReadHeader()noexcept;
		void ReadBody( uint messageLength )noexcept;
		virtual void Process( google::protobuf::uint8* pData, uint size )noexcept=0;

		std::atomic<bool> _connected{false};
		std::unique_ptr<basio::ip::tcp::socket> _pSocket;
		//basio::ip::tcp::socket* _pSocket{nullptr};
		char _readMessageSize[4];
		vector<google::protobuf::uint8> _message;
	};
#pragma warning(push)
#pragma warning( disable : 4459 )
	struct ProtoClient : public PerpetualAsyncSocket, public ProtoClientSession
	{
		ProtoClient( sv host, uint16 port, sv name )noexcept;
		virtual ~ProtoClient()=default;
		void Connect()noexcept;
		const string Name;
	protected:
		void Startup( sv clientThreadName )noexcept;
	private:
		void Connect( const basio::ip::tcp::resolver::results_type& endpoints )noexcept(false);
		void Run( sv name )noexcept;
		string _host;
		uint16 _port;
	};
#pragma warning(pop)

	template<typename TOut, typename TIn>
	struct TProtoClient : ProtoClient
	{
		TProtoClient( sv host, uint16 port, sv socketThreadName )noexcept:
			ProtoClient{host, port, socketThreadName}
		{}
   	virtual ~TProtoClient()=default;
		void Process( google::protobuf::uint8* pData, uint size )noexcept override;
		void Write( sp<TOut> message )noexcept;
		virtual void OnReceive( sp<TIn> pIn )=0;
	private:
		//void ReadBody()noexcept;
		void Write2()noexcept(false);
		Queue<TOut> _writeMessages;
	};
#define var const auto
	template<typename TOut, typename TIn>
	void TProtoClient<TOut,TIn>::Process( google::protobuf::uint8* pData, uint size )noexcept
	{
		google::protobuf::io::CodedInputStream input( pData, (int)size );
		auto pTransmission = make_shared<TIn>();
		if( !pTransmission->MergePartialFromCodedStream(&input) )
			THROW( IOException("MergePartialFromCodedStream returned false") );
		OnReceive( pTransmission );
	}

	template<typename TOut, typename TIn>
	void TProtoClient<TOut,TIn>::Write( shared_ptr<TOut> msg )noexcept
	{
		auto onPost = [this, msg]()
		{
			bool writeInProgress = !_writeMessages.Empty();
			_writeMessages.Push( msg );
			if( !writeInProgress )
				try{ Write2(); }catch(...){}
		};
		basio::post( _asyncHelper, onPost );
//TODO take out
		// TOut out;
		// if( out.MergePartialFromCodedStream(&input) )
		// 	THROW( IOException("MergePartialFromCodedStream returned false, length={}", pData->size()) );
	}

	template<typename TOut, typename TIn>
	void TProtoClient<TOut,TIn>::Write2()noexcept(false)
	{
		var pMessage = _writeMessages.TryPop();
		if( !pMessage )
		{
			CRITICAL0( "no message to write."sv );
			return;
		}
		var pData = ProtoClientSession::ToBuffer( *pMessage );
		// for( var& item : pMessage->messages() )
		// {
		// 	//Do strings/other
		// 	if( item.has_message() )
		// 	{
		// 		var& message = item.message();
		// 		DBG( "message_id:  {}", message.messageid() );
		// 		for( auto i=0; i<message.variables_size(); ++i )
		// 			DBG( "Variable:  {}", message.variables(i) );
		// 	}
		// 	else if( item.has_string() )
		// 		DBG( "id:  {}, field: {}, value:  {}.", item.string().id(), item.string().field(), item.string().value() );
		// }
		auto onDone = [this, pData](std::error_code ec, std::size_t /*length*/)
		{
			if( ec )
			{
				Jde::GetDefaultLogger()->error( "Read Body Failed - {}", ec.value() );
				if( _pSocket )
					_pSocket->close();
				_pSocket = nullptr;
			}
			else
			{
				//_writeMessages.TryPop();
				if( !_writeMessages.Empty() )
					Write2();
			}
		};
		TRACEX( "Writing:  {} - count={}"sv, pData->size(), pMessage->messages_size() );
#ifndef NDEBUG
		//google::protobuf::io::CodedInputStream input( pData->data()+4, pData->size()-4 );
		//auto pTransmission = make_shared<TOut>();
		//if( !pTransmission->MergePartialFromCodedStream(&input) )
		//	THROW( IOException("MergePartialFromCodedStream returned false, length={}", pData->size()) );
#endif
		auto buffer = basio::buffer( pData->data(), pData->size() );
		if( _pSocket )
			basio::async_write( *_pSocket, buffer, onDone );
	}
#undef var
}