#include "CoWorker.h"
#include "../threading/InterruptibleThread.h"

namespace Jde::Coroutine
{
	static const LogTag& _logLevel{ Logging::TagLevel("threads") };
	sp<CoWorker> CoWorker::_pInstance;
	std::once_flag CoWorker::_singleThread;

	α CoWorker::Start()noexcept->void
	{
		IApplication::AddShutdown( _pInstance );
		_pThread = make_unique<Threading::InterruptibleThread>( _name, [this](){Run();} );
	}
	α CoWorker::Shutdown()noexcept->void
	{
		_pThread->Interrupt();
		{
			std::unique_lock<std::mutex> lk( _mtx );
			_cv.notify_one();
		}
		_pThread->Join();
		_pInstance = nullptr;
	}

	α CoWorker::Run()noexcept->void
	{
		Threading::SetThreadDscrptn( _name );
		LOG( "{} - Starting", _name );
		while( !Threading::GetThreadInterruptFlag().IsSet() )
		{
			Process();
		}
		LOG( "{} - Ending", _name );
	}
}