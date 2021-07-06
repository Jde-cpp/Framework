#include "Worker.h"
#include "Thread.h"
#include "InterruptibleThread.h"
#define var const auto
namespace Jde::Threading
{
	sp<IWorker> IWorker::_pInstance;
	std::atomic<bool> IWorker::_mutex;
	IWorker::IWorker( sv name )noexcept:
		_name{name},
		_pThread{ make_unique<jthread>([&]( stop_token st ){_pInstance->Run( st );}) }
	{}//DBG("IWorker::IWorker({})"sv, _name); 
	
	IWorker::~IWorker(){}//abstract

	α IWorker::Shutdown()noexcept->void
	{
		Threading::AtomicGuard l{ _mutex };
		if( !_pInstance )
			return;
		_pThread->request_stop();
		_pThread->join();
		_pThread = nullptr;
		_pInstance = nullptr;
	}
	void IWorker::Run( stop_token st )noexcept
	{
		Threading::SetThreadDscrptn( _name );
		sp<IWorker> pKeepAlive;
		var keepAlive = Settings::Get<Duration>( "WorkerkeepAlive" ).value_or( 5s );
		DBG( "{} - Starting"sv, _name );
		while( !st.stop_requested() )
		{
			if( Poll() )
				continue;
			if( Clock::now()>_lastRequest+keepAlive )
			{
				Threading::AtomicGuard l{ _mutex };
				DBG( "useCount={}"sv, _pInstance.use_count() );
				if( _pInstance.use_count()<4 )//Shutdown & IApp::Objects & _pInstance
				{
					std::swap( pKeepAlive, _pInstance );
					IApplication::RemoveShutdown( pKeepAlive );
					_pThread->request_stop();
				}
				else
					_lastRequest = Clock::now()+keepAlive;
			}
			else
				std::this_thread::yield();
		}
		DBG( "{} - Ending"sv, _name );
	}
}