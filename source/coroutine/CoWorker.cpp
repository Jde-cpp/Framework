#include "CoWorker.h"
#include "../threading/InterruptibleThread.h"

namespace Jde::Coroutine
{
	sp<CoWorker> CoWorker::_pInstance;
	std::once_flag CoWorker::_singleThread;


	void CoWorker::Start()noexcept
	{
	//	_pInstance = shared_from_this();
		IApplication::AddShutdown( _pInstance );
		_pThread = make_unique<Threading::InterruptibleThread>( _name, [this](){Run();} );
	}
	void CoWorker::Shutdown()noexcept
	{
		_pThread->Interrupt();
		std::unique_lock<std::mutex> lk( _mtx );
		_cv.notify_one();
		_pThread->Join();
		_pInstance = nullptr;
	}

	void CoWorker::Run()noexcept
	{
		Threading::SetThreadDscrptn( _name );
		DBG( "{} - Starting"sv, _name );
		while( !Threading::GetThreadInterruptFlag().IsSet() )
		{
			Process();
		}
		DBG( "{} - Ending"sv, _name );
	}
}