#pragma once
#include <jde/App.h>
#include <jde/coroutine/Task.h>
#include "../threading/Thread.h"

namespace Jde::Coroutine
{
	typedef uint ClientHandle;
	struct Γ CoWorker /*abstract*/: public IShutdown, std::enable_shared_from_this<CoWorker>
	{
		CoWorker( sv name )noexcept:_name{name}{};
		void Shutdown()noexcept override;
		void Start()noexcept;
	protected:
		virtual void Process()noexcept=0;
		const string _name;
		up<Threading::InterruptibleThread> _pThread;
		std::condition_variable _cv; mutable std::mutex _mtx;
		static sp<CoWorker> _pInstance;
		static std::once_flag _singleThread;
	private:
		void Run()noexcept;
	};

	template<typename TDerived,typename TAwaitable>
	struct TCoWorker /*abstract*/: CoWorker
	{
		TCoWorker( sv name )noexcept:CoWorker{name}{};
		template<typename TAwaitable2=TAwaitable>
		struct Handles{ coroutine_handle<Task::promise_type> HCo; Coroutine::ClientHandle HClient; };
		virtual ~TCoWorker()noexcept{ DBG("TCoWorker::~TCoWorker({})"sv, _name); }
		static sp<TDerived> Instance()noexcept{ return std::static_pointer_cast<TDerived>(_pInstance); }
	protected:
		static constexpr Duration WakeDuration{5s};
	};
}