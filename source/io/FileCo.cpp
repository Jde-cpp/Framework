#include "FileCo.h"
#include <signal.h>

#define var const auto
namespace Jde::IO
{
	α IFileChunkArg::Handle()noexcept->HFile&{ return _fileIOArg.Handle; }
	uint32 _chunkSize=0;
	α DriveWorker::ChunkSize()noexcept->uint32{ return _chunkSize==0 ? (_chunkSize=Settings::TryGet<uint32>("workers/drive/chunkSize").value_or(1 << 19)) : _chunkSize; }

	uint8 _threadSize=0;
	α DriveWorker::ThreadSize()noexcept->uint8{ return _threadSize==0 ? (_threadSize=Settings::TryGet<uint8>("workers/drive/threadSize").value_or(5)) : _threadSize; }

	//uint DriveWorker::Signal{ SIGUSR1+1 };
	void DriveWorker::Initialize()noexcept
	{
		IWorker::Initialize();
	}

	FileIOArg::FileIOArg( path path, bool vec )noexcept:
		IsRead{ true },
		Path{ path }
	{
		if( vec )
			Buffer = make_shared<vector<char>>();
		else
			Buffer = make_shared<string>();
	}
	FileIOArg::FileIOArg( path path, sp<vector<char>> pVec )noexcept:
		Path{ path }, 
		Buffer{ pVec }
	{}
	FileIOArg::FileIOArg( path path, sp<string> pData )noexcept:
		Path{ path }, 
		Buffer{ pData }
	{}

/*	void FileIOArg::Send( coroutine_handle<Task2::promise_type>&& h )noexcept
	{
//		DBG( "FileIOArg::Send max={}"sv, _SC_AIO_LISTIO_MAX );
		CoHandle = move( h );
		var size = std::visit( [](auto&& x){return x->size();}, Buffer );
		var chunkSize = DriveWorker::ChunkSize;
		Chunks.reserve( size/chunkSize+1 );
		for( uint32 i=0; i<size; i+=chunkSize )
			Chunks.push_back( CreateChunk(i, std::min(chunkSize, size)) );
		for( uint8 i=0; i<Chunks.size() && i<DriveWorker::ThreadSize; ++i )
			Chunks[i]->Sent = true;
		for( uint8 i=0; i<Chunks.size() && i<DriveWorker::ThreadSize; ++i )
			Chunks[i]->Process();
//		DBG( "~FileIOArg::Send"sv );
	}
*/
	//atomic<bool> _mutex;
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

	α DriveAwaitable::await_ready()noexcept->bool
	{
		try
		{
			_arg.Open();
		}
		catch( const IOException& e )
		{
			ExceptionPtr = std::make_exception_ptr( e );
		}
		return ExceptionPtr!=nullptr;
	}
	α DriveAwaitable::await_suspend( typename base::THandle h )noexcept->void
	{
		base::await_suspend( h );
		_arg.Send( move(h) );
	}
	α DriveAwaitable::await_resume()noexcept->TaskResult
	{
		base::AwaitResume();
		return _pPromise ? TaskResult{ _pPromise->get_return_object().GetResult() } : TaskResult{ ExceptionPtr };
	}
}