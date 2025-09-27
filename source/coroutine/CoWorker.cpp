#include "CoWorker.h"
#include "../threading/InterruptibleThread.h"
#ifdef Unused
namespace Jde::Coroutine{
	constexpr ELogTags _tags = ELogTags::Threads;
	sp<CoWorker> CoWorker::_pInstance;
	std::once_flag CoWorker::_singleThread;

	α CoWorker::Start()ι->void{
		Process::AddShutdown( _pInstance );
		_pThread = mu<Threading::InterruptibleThread>( _name, [this](){Run();} );
	}

	α CoWorker::Shutdown( bool /*terminate*/ )ι->void{
		Trace( _tags, "({})Shutdown", _name );
		_pThread->Interrupt();
		{
			std::unique_lock<std::mutex> lk( _mtx );
			_cv.notify_one();
		}
		_pThread->Join();
		_pInstance = nullptr;
		Trace( _tags, "({})~Shutdown", _name );
	}

	α CoWorker::Run()ι->void{
		SetThreadDscrptn( _name );
		Trace( _tags, "{} - Starting", _name );
		while( !Threading::GetThreadInterruptFlag().IsSet() ){
			Process();
		}
		Trace( _tags, "{} - Ending", _name );
	}
}
#endif