#include "FileCo.h"
#include "../Cache.h"
#include <signal.h>

#define var const auto
namespace Jde::IO
{
	α IFileChunkArg::Handle()noexcept->HFile&{ return _fileIOArg.Handle; }
	uint32 _chunkSize=0;
	α DriveWorker::ChunkSize()noexcept->uint32{ return _chunkSize==0 ? (_chunkSize=Settings::Get<uint32>("workers/drive/chunkSize").value_or(1 << 19)) : _chunkSize; }

	uint8 _threadSize=0;
	α DriveWorker::ThreadSize()noexcept->uint8{ return _threadSize==0 ? (_threadSize=Settings::Get<uint8>("workers/drive/threadSize").value_or(5)) : _threadSize; }

	void DriveWorker::Initialize()noexcept
	{
		IWorker::Initialize();
	}

	FileIOArg::FileIOArg( fs::path path, bool vec )noexcept:
		IsRead{ true },
		Path{ move(path) }
	{
		if( vec )
			Buffer = make_shared<vector<char>>();
		else
			Buffer = make_shared<string>();
	}
	FileIOArg::FileIOArg( fs::path path, sp<vector<char>> pVec )noexcept:
		Path{ move(path) },
		Buffer{ pVec }
	{}
	FileIOArg::FileIOArg( fs::path path, sp<string> pData )noexcept:
		Path{ move(path) },
		Buffer{ pData }
	{}

	bool FileIOArg::HandleChunkComplete( IFileChunkArg* pChunkArg )noexcept
	{
		//Threading::AtomicGuard l{ _mutex };
		IFileChunkArg* pNextChunk{ nullptr };
		up<IFileChunkArg> pChunk;
		bool additional{ false };
		for( auto pp=Chunks.begin(); !(pChunk && pNextChunk) && pp!=Chunks.end(); ++pp )
		{
			if( *pp==nullptr )
				continue;
			if( (*pp).get()==pChunkArg )
				pChunk = move( *pp );
			else if( !pNextChunk && !(*pp)->Sent.exchange(true) )
				pNextChunk = (*pp).get();
			else
				additional = true;
		}
		ASSERT( pChunk );
		if( pNextChunk )
			pNextChunk->Process();
		else
		{
			//DBG( "[{}] close"sv, pChunkArg->Handle() );
			//::close( pChunkArg->Handle() );//TODO fix this on windows.
		}
		return !pNextChunk && !additional;
	}
	// α FileIOArg::Send( coroutine_handle<Task2::promise_type>&& h )noexcept->void
	// {
	// 	CoHandle = move( h );
	// 	for( uint i=0; i*DriveWorker::ChunkSize()<Size(); ++i )
	// 		Chunks.emplace_back( CreateChunk(i) );
	// 	OSSend();
	// }

	α DriveAwaitable::await_ready()noexcept->bool
	{
		bool cache = false;
		try
		{
			if( auto p = _cache ? Cache::Get<sp<void>>(_arg.Path.string()) : sp<void>{}; p )//TODO lock the file
			{
				cache = true;
				if( auto pT = _arg.Buffer.index()==0 ? static_pointer_cast<vector<char>>(p) : sp<vector<char>>{}; pT )
					_arg.Buffer = pT;
				else if( auto pT = _arg.Buffer.index()==1 ? static_pointer_cast<string>(p) : sp<string>{}; pT )
					_arg.Buffer = pT;
				else
				{
					cache = false;
					ERR( "could not cast cache vector='{}'", _arg.Buffer.index()==0 );
				}
			}
			if( !cache )
				_arg.Open();
		}
		catch( IOException& e )
		{
			ExceptionPtr = e.Clone();
		}
		return ExceptionPtr!=nullptr || cache;
	}
	α DriveAwaitable::await_suspend( HCoroutine h )noexcept->void
	{
		base::await_suspend( h );
		_arg.Send( move(h) );
	}
}