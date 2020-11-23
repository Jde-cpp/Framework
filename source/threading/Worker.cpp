#include "Worker.h"
#include "Thread.h"
#include "InterruptibleThread.h"

namespace Jde::Threading
{
	sp<Worker> Worker::_pInstance;
	Worker::Worker( sv name )noexcept:
		_name{ name }
	{}

	void Worker::Start()noexcept
	{
		_pInstance = shared_from_this();
		IApplication::AddShutdown( _pInstance );
		_pThread = make_unique<Threading::InterruptibleThread>( _name, [this](){Run();} );
	}
	void Worker::Shutdown()noexcept
	{
		_pThread->Interrupt();
		_pThread->Join();
		_pInstance = nullptr;
	}

	void Worker::Run()noexcept
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