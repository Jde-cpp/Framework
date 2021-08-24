#pragma once
#include <jde/coroutine/Task.h>
#include "../threading/Worker.h"
#ifdef _MSC_VER
	using HFile=DWORD;
#else
	using HFile=int;
#endif
namespace Jde::IO
{
	using namespace Coroutine;
	struct FileIOArg;
	struct IFileChunkArg
	{
		IFileChunkArg( FileIOArg& ioArg ):index{Index++},_fileIOArg{ioArg}{}
		virtual ~IFileChunkArg()=default;
		//virtual uint StartIndex()const noexcept=0; virtual void SetStartIndex( uint i )noexcept=0;
		//virtual uint EndIndex()const noexcept=0; virtual void SetEndIndex( uint i )noexcept=0;
		virtual uint Bytes()const noexcept=0; virtual void SetBytes( uint i )noexcept=0;
		//virtual void SetFileIOArg( FileIOArg* p )noexcept=0;
		virtual HFile Handle()noexcept=0;
		virtual void Process()noexcept{};
		FileIOArg& FileArg()const noexcept{ return _fileIOArg; }

		atomic<bool> Sent{false};
		uint index{500};
		static uint Index;
	private:
		struct FileIOArg& _fileIOArg;
	};

	struct FileIOArg final//: boost::noncopyable
	{
		FileIOArg( path path, bool vec ):
			IsRead{ true },
			Path{ path }
		{
			if( vec )
				Buffer = make_shared<vector<char>>();
			else
				Buffer = make_shared<string>();
		}
		FileIOArg( path path, sp<vector<char>> pVec ):
			Path{ path }, Buffer{ pVec }
		{}
		FileIOArg( path path, sp<string> pData ):
			Path{ path }, Buffer{ pData }
		{}
		~FileIOArg(){ DBG("FileIOArg::~FileIOArg"sv); }
		//up<IFileChunkArg> CreateChunk( uint startIndex, uint endIndex )noexcept;
		void Open()noexcept(false);
		//HFile Handle()noexcept;
		//HFile SubsequentHandle()noexcept;
		bool HandleChunkComplete( IFileChunkArg* pChunkArg )noexcept;
		//void Send( coroutine_handle<Task2::promise_type>&& h )noexcept;
		//void SendNext()noexcept;
		void SetWorker( sp<Threading::IWorker> p ){ _pWorkerKeepAlive=p; }
		/*const*/ bool IsRead{false};
		char* Data()noexcept{ return std::visit( [](auto&& x){return x->data();}, Buffer ); }

		fs::path Path;
		std::variant<sp<vector<char>>,sp<string>> Buffer;

		vector<up<IFileChunkArg>> Chunks;
		coroutine_handle<Task2::promise_type> CoHandle;
		HFile Handle{0};
	private:

		sp<Threading::IWorker> _pWorkerKeepAlive;
	};

	struct DriveAwaitable : IAwaitable
	{
		using base=IAwaitable;
		DriveAwaitable( path path, bool vector )noexcept:_arg{ path, vector }{ DBG("DriveAwaitable::Read"sv); }
		DriveAwaitable( path path, sp<vector<char>> data )noexcept:_arg{ path, data }{ DBG("DriveAwaitable::Write"sv); }
		DriveAwaitable( path path, sp<string> data )noexcept:_arg{ path, data }{ DBG("here"sv); }
		bool await_ready()noexcept override;
		void await_suspend( typename base::THandle h )noexcept override;//{ base::await_suspend( h ); _pPromise = &h.promise(); }
		TaskResult await_resume()noexcept override;
	private:
		std::exception_ptr ExceptionPtr;
		FileIOArg _arg;
	};

	struct DriveWorker : Threading::IWorker
	{
		using base=Threading::IWorker;
		DriveWorker():base{"DriveWorker"}{}
		void Initialize()noexcept override;
		//~DriveWorker(){ DBG("~DriveWorker"sv); }
		static uint32 ChunkSize;
		static uint8 ThreadSize;
		static uint Signal;
	};
}