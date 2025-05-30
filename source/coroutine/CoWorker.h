#pragma once
#include <jde/framework/process.h>
#include <jde/framework/coroutine/TaskOld.h>
#include "../threading/Thread.h"
#include "../threading/InterruptibleThread.h"

namespace Jde::Coroutine{

	typedef uint ClientHandle;
	struct Γ CoWorker /*abstract*/: public IShutdown, std::enable_shared_from_this<CoWorker>{
		CoWorker( sv name )ι:_name{name}{};
		void Shutdown( bool terminate )ι override;
		void Start()ι;
	protected:
		virtual void Process()ι=0;
		const string _name;
		up<Threading::InterruptibleThread> _pThread;
		std::condition_variable _cv; mutable std::mutex _mtx;
		static sp<CoWorker> _pInstance;
		static std::once_flag _singleThread;
	private:
		void Run()ι;
	};

	template<typename TDerived,typename TAwaitable>
	struct TCoWorker /*abstract*/: CoWorker
	{
		TCoWorker( sv name )ι:CoWorker{name}{};
		template<typename TAwaitable2=TAwaitable>
		struct Handles{ coroutine_handle<Task::promise_type> HCo; Coroutine::ClientHandle HClient; };
		virtual ~TCoWorker()ι{ /*DBGT("TCoWorker::~TCoWorker({})"sv, _name);*/ }
		static sp<TDerived> Instance()ι{ return std::static_pointer_cast<TDerived>(_pInstance); }
	protected:
		static constexpr Duration WakeDuration{5s};
	};
}