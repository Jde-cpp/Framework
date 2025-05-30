#include "FileCo.h"
#include "../Cache.h"
#include <signal.h>

#define let const auto
namespace Jde::IO{
	constexpr ELogTags _tags = ELogTags::IO;
	α IFileChunkArg::Handle()ι->HFile&{ return _fileIOArg.Handle; }
	uint32 _chunkSize=0;
	α DriveWorker::ChunkSize()ι->uint32{ return _chunkSize==0 ? (_chunkSize=Settings::FindNumber<uint32>("/workers/drive/chunkSize").value_or(1 << 19)) : _chunkSize; }

	uint8 _threadSize=0;
	α DriveWorker::ThreadSize()ι->uint8{ return _threadSize==0 ? (_threadSize=Settings::FindNumber<uint8>("/workers/drive/threadSize").value_or(5)) : _threadSize; }

	void DriveWorker::Initialize()ι{
		IWorker::Initialize();
	}

	FileIOArg::FileIOArg( fs::path path, bool vec, SL sl )ι:
		IsRead{ true },
		Path{ move(path) },
		_sl{ sl }{
		if( vec )
			Buffer = make_shared<vector<char>>();
		else
			Buffer = make_shared<string>();
	}
	FileIOArg::FileIOArg( fs::path path, sp<vector<char>> pVec, SL sl )ι:
		Path{ move(path) },
		Buffer{ pVec },
		_sl{ sl }
	{}
	FileIOArg::FileIOArg( fs::path path, sp<string> pData, SL sl )ι:
		Path{ move(path) },
		Buffer{ pData },
		_sl{ sl }
	{}

	bool FileIOArg::HandleChunkComplete( IFileChunkArg* pChunkArg )ι{
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
	// α FileIOArg::Send( coroutine_handle<Task2::promise_type>&& h )ι->void
	// {
	// 	CoHandle = move( h );
	// 	for( uint i=0; i*DriveWorker::ChunkSize()<Size(); ++i )
	// 		Chunks.emplace_back( CreateChunk(i) );
	// 	OSSend();
	// }

	α DriveAwaitable::await_ready()ι->bool{
		bool cache = false;
		try
		{
			let path = _arg.Path.string();
			if( auto p = _cache ? Cache::Get<sp<void>>(path) : sp<void>{}; p ){//TODO lock the file
				Trace( _tags, "({})Open from cache", path );
				cache = true;
				if( auto pT = _arg.Buffer.index()==0 ? static_pointer_cast<vector<char>>(p) : sp<vector<char>>{}; pT )
					_arg.Buffer = pT;
				else if( auto pT = _arg.Buffer.index()==1 ? static_pointer_cast<string>(p) : sp<string>{}; pT )
					_arg.Buffer = pT;
				else{
					cache = false;
					Error( _tags, "could not cast cache vector='{}'", _arg.Buffer.index()==0 );
				}
			}
			if( !cache )
				_arg.Open();
		}
		catch( IOException& e ){
			ExceptionPtr = e.Move();
		}
		return ExceptionPtr!=nullptr || cache;
	}

	α DriveAwaitable::Suspend()ι->void{
		_arg.Send( _h );
	}
}