#pragma once
#ifndef FILECO_H
#define FILECO_H
#include <jde/framework/coroutine/Task.h>
#include "../threading/Worker.h"
#include "../coroutine/Awaitable.h"
#ifdef _MSC_VER
	#include <jde/framework/process/os/windows/WindowsHandle.h>
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
		//virtual uint StartIndex()Ι=0; virtual void SetStartIndex( uint i )ι=0;
		//virtual uint EndIndex()Ι=0; virtual void SetEndIndex( uint i )ι=0;
		//β Bytes()Ι->uint=0; virtual void SetBytes( uint i )ι=0;
		//virtual void SetFileIOArg( FileIOArg* p )ι=0;
		β Handle()ι->HFile&;
		β Process()ι->void{};
		β FileArg()ι->FileIOArg&{ return _fileIOArg;}
		β FileArg()Ι->const FileIOArg&{ return _fileIOArg;}
		//β SetFileArg( const FileIOArg* )Ι->void=0;

		std::atomic<bool> Sent;
		uint Index;
		//static uint Index;
//	private:
		FileIOArg& _fileIOArg;
	};

	struct FileIOArg final//: boost::noncopyable
	{
		FileIOArg( fs::path path, bool vec, SRCE )ι;
		FileIOArg( fs::path path, sp<vector<char>> pVec, SRCE )ι;
		FileIOArg( fs::path path, sp<string> pData, SRCE )ι;
		//~FileIOArg(){ DBG("FileIOArg::~FileIOArg"sv); }
		α Open()ε->void;
		α HandleChunkComplete( IFileChunkArg* pChunkArg )ι->bool;
		α Send( coroutine_handle<Task::promise_type> h )ι->void;
		α SetWorker( sp<Threading::IWorker> p ){ _pWorkerKeepAlive=p; }
		α Data()ι{ return std::visit( [](auto&& x){return x->data();}, Buffer ); }
		α Size()Ι{ return std::visit( [](auto&& x){return x->size();}, Buffer ); }

		/*const*/ bool IsRead{ false };
		fs::path Path;
		std::variant<sp<vector<char>>,sp<string>> Buffer;
		vector<up<IFileChunkArg>> Chunks;
		coroutine_handle<Task::promise_type> CoHandle;
		HFile Handle{0};
		source_location _sl;
	private:
		sp<Threading::IWorker> _pWorkerKeepAlive;
	};

	struct Γ DriveAwaitable final : IAwait{
		using base=IAwait;
		DriveAwaitable( fs::path path, bool vector, bool cache, SRCE )ι:base{ sl },_arg{ move(path), vector },_cache{cache}{}
		DriveAwaitable( fs::path path, sp<vector<char>> data, SRCE )ι:base{ sl },_arg{ move(path), data },_cache{false}{}
		DriveAwaitable( fs::path path, sp<string> data, SRCE )ι:base{ sl },_arg{ move(path), data },_cache{false}{}
		α await_ready()ι->bool override;
		α Suspend()ι->void override;//{ base::await_suspend( h ); _pPromise = &h.promise(); }
		α await_resume()ι->AwaitResult override;
	private:
		up<IException> ExceptionPtr;
		FileIOArg _arg;
		bool _cache;
	};

	struct DriveWorker : Threading::IPollWorker
	{
		using base=Threading::IPollWorker;
		DriveWorker():base{"drive"}{}
		α Initialize()ι->void override;
		Γ Ω ChunkSize()ι->uint32;
		Γ Ω ThreadSize()ι->uint8;
		Γ Ω Signal()ι->uint;
	};
}
#endif