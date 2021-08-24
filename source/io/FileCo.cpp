#include "FileCo.h"
#include <signal.h>

#define var const auto
namespace Jde::IO
{
	uint IFileChunkArg::Index{0};
	uint32 DriveWorker::ChunkSize{ 1 << 19 };
	uint8 DriveWorker::ThreadSize{ 5 };
	uint DriveWorker::Signal{ SIGUSR1+1 };
	void DriveWorker::Initialize()noexcept
	{
		IWorker::Initialize();
		var pSettings = IWorker::Settings();
		if( pSettings )
		{
			ChunkSize = pSettings->Get2<uint32>("chunkSize").value_or( DriveWorker::ChunkSize );
			ThreadSize = pSettings->Get2<uint8>("threadSize").value_or( DriveWorker::ThreadSize );
		}
	}
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

	bool DriveAwaitable::await_ready()noexcept
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
/*	void DriveAwaitable::await_suspend( typename base::THandle h )noexcept
	{
		base::await_suspend( h );
		_arg.Send( move(h) );
	}
	TaskResult DriveAwaitable::await_resume()noexcept
	{
		base::AwaitResume();
		return _pPromise ? TaskResult{ _pPromise->get_return_object().GetResult() } : TaskResult{ ExceptionPtr };
	}
*/
}