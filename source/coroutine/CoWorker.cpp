#include "CoWorker.h"
#include "../threading/InterruptibleThread.h"

namespace Jde::Coroutine
{
	static const sp<LogTag> _logTag{ Logging::Tag("threads") };
	sp<CoWorker> CoWorker::_pInstance;
	std::once_flag CoWorker::_singleThread;

	α CoWorker::Start()ι->void
	{
		IApplication::AddShutdown( _pInstance );
		_pThread = make_unique<Threading::InterruptibleThread>( _name, [this](){Run();} );
	}
	α CoWorker::Shutdown()ι->void
	{
		TRACE( "({})Shutdown", _name );
		_pThread->Interrupt();
		{
			std::unique_lock<std::mutex> lk( _mtx );
			_cv.notify_one();
		}
		_pThread->Join();
		_pInstance = nullptr;
		TRACE( "({})~Shutdown", _name );
	}

	α CoWorker::Run()ι->void
	{
		Threading::SetThreadDscrptn( _name );
		TRACE( "{} - Starting", _name );
		while( !Threading::GetThreadInterruptFlag().IsSet() )
		{
			Process();
		}
		TRACE( "{} - Ending", _name );
	}
}