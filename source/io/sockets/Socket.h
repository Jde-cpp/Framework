#pragma once
#include <deque>
#include "../../Exports.h"
#include "../../application/Application.h"


namespace Jde::Threading{ struct InterruptibleThread; }

namespace Jde::IO::Sockets
{
//#ifdef __GNUC__
//	namespace basio=asio;
//#else
	namespace basio=boost::asio;
//#endif
/*	typedef std::function<void(std::error_code, size_t)> OnDone;
	class SessionCallbacks;

	class Session
	{
	public:
		Session()=default;
		virtual ~Session()=default;
		virtual void Write( sp<OutgoingMessage> message )noexcept=0;
		virtual void Write( sp<OStreamBuffer> message )noexcept=0;
		virtual void Write( sp<OStreamBuffer> message, OnDone onDone )noexcept=0;
		//virtual void Write( char* pStart, uint length, OnDone onDone )noexcept=0;
		template<typename T>
		void Write( const T& object, OnDone onDone )noexcept;
		template<typename T>
		void WriteObject( const T& object, uint type=0 )noexcept;
		virtual uint Id()const{return 0;}
	protected:
		virtual basio::ip::tcp::socket& GetSocket()=0;
		virtual void ReadHeader()noexcept=0;
		virtual void ReadBody()noexcept=0;
		uint_fast32_t MessageLength()const noexcept;
		char _readMessageSize[4];
	};
	*/
	struct JDE_NATIVE_VISIBILITY AsyncSocket : public IShutdown
	{
		virtual ~AsyncSocket();
	protected:
		AsyncSocket()noexcept;
		//std::unique_ptr<boost::asio::io_context> _pContext;
		boost::asio::io_context _asyncHelper;
		void Join();
		void RunAsyncHelper()noexcept;
		void Close()noexcept;
		virtual void OnClose()noexcept{};
		void Shutdown()noexcept;
	private:
		void Run()noexcept;
		sp<Threading::InterruptibleThread> _pThread;
		string _threadName;
	};

	class PerpetualAsyncSocket : protected AsyncSocket
	{
	public:
		virtual ~PerpetualAsyncSocket()=default;
	protected:
		PerpetualAsyncSocket()noexcept;
		basio::executor_work_guard<basio::io_context::executor_type> _keepAlive;
	};
	/*
	template<typename T>
	sp<OStreamBuffer> SessionToBuffer( const T& object, uint type=0 )noexcept
	{
		auto pBuffer = std::make_shared<OStreamBuffer>( std::make_unique<std::vector<char>>(8192) ); 
		pBuffer->WriteLength();
		std::ostream os( pBuffer.get() );
		if( type!=0 )
			os << type;
		object.Write( os );
		return pBuffer;
	}
	
	template<typename T> 
	void Session::Write( const T& object, OnDone onDone )noexcept
	{
		Write( SessionToBuffer(object), onDone );
	}
/ *template<typename T>
	void Session::Write( const T& object )noexcept
	{
		
	}*/
	/*
	template<typename T>
	void Session::WriteObject( const T& object, uint type )noexcept
	{
		Write( SessionToBuffer(object, type) );
	}
	*/
}