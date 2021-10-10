#pragma once
#include <jde/coroutine/Task.h>
#include "../threading/Worker.h"
#ifdef _MSC_VER
	#include "../../../Windows/source/WindowsHandle.h"
	using HFile=Jde::HandlePtr;
#else
	using HFile=int;
#endif
namespace Jde::IO
{
	using namespace Coroutine;
	struct FileIOArg;
	struct IFileChunkArg
	{
		IFileChunkArg( FileIOArg& arg, uint index ):
			Index{index},
			_fileIOArg{arg}
		{}
		virtual ~IFileChunkArg()=default;
		//virtual uint StartIndex()const noexcept=0; virtual void SetStartIndex( uint i )noexcept=0;
		//virtual uint EndIndex()const noexcept=0; virtual void SetEndIndex( uint i )noexcept=0;
		//virtual α Bytes()const noexcept->uint=0; virtual void SetBytes( uint i )noexcept=0;
		//virtual void SetFileIOArg( FileIOArg* p )noexcept=0;
		virtual α Handle()noexcept->HFile&;
		virtual α Process()noexcept->void{};
		virtual α FileArg()noexcept->FileIOArg&{ return _fileIOArg;}
		virtual α FileArg()const noexcept->const FileIOArg&{ return _fileIOArg;}
		//virtual α SetFileArg( const FileIOArg* )const noexcept->void=0;

		atomic<bool> Sent{false};
		uint Index;
		//static uint Index;
//	private:
		FileIOArg& _fileIOArg;
	};

	struct FileIOArg final//: boost::noncopyable
	{
		FileIOArg( path path, bool vec )noexcept;
		FileIOArg( path path, sp<vector<char>> pVec )noexcept;
		FileIOArg( path path, sp<string> pData )noexcept;
		~FileIOArg(){ DBG("FileIOArg::~FileIOArg"sv); }
		//up<IFileChunkArg> CreateChunk( uint startIndex, uint endIndex )noexcept;
		α Open()noexcept(false)->void;
		//HFile Handle()noexcept;
		//HFile SubsequentHandle()noexcept;
		α HandleChunkComplete( IFileChunkArg* pChunkArg )noexcept->bool;
		α Send( coroutine_handle<Task2::promise_type>&& h )noexcept->void;
		//void SendNext()noexcept;
		α SetWorker( sp<Threading::IWorker> p ){ _pWorkerKeepAlive=p; }
		α Data()noexcept{ return std::visit( [](auto&& x){return x->data();}, Buffer ); }
		α Size()const noexcept{ return std::visit( [](auto&& x){return x->size();}, Buffer ); }

		/*const*/ bool IsRead{false};
		fs::path Path;
		std::variant<sp<vector<char>>,sp<string>> Buffer;

		vector<up<IFileChunkArg>> Chunks;
		coroutine_handle<Task2::promise_type> CoHandle;
		HFile Handle{0};

	private:
		sp<Threading::IWorker> _pWorkerKeepAlive;
	};

	struct JDE_NATIVE_VISIBILITY DriveAwaitable : IAwaitable
	{
		using base=IAwaitable;
		DriveAwaitable( path path, bool vector )noexcept:_arg{ path, vector }{ DBG("DriveAwaitable::Read"sv); }
		DriveAwaitable( path path, sp<vector<char>> data )noexcept:_arg{ path, data }{ DBG("DriveAwaitable::Write"sv); }
		DriveAwaitable( path path, sp<string> data )noexcept:_arg{ path, data }{ DBG("here"sv); }
		α await_ready()noexcept->bool override;
		α await_suspend( typename base::THandle h )noexcept->void override;//{ base::await_suspend( h ); _pPromise = &h.promise(); }
		α await_resume()noexcept->TaskResult override;
	private:
		std::exception_ptr ExceptionPtr;
		FileIOArg _arg;
	};

	struct DriveWorker : Threading::IPollWorker
	{
		using base=Threading::IPollWorker;
		DriveWorker():base{"drive"}{}
		α Initialize()noexcept->void override;
		JDE_NATIVE_VISIBILITY Ω ChunkSize()noexcept->uint32;
		JDE_NATIVE_VISIBILITY Ω ThreadSize()noexcept->uint8;
		JDE_NATIVE_VISIBILITY Ω Signal()noexcept->uint;
	};
}