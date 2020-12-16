#pragma once
#include "../application/Application.h"
#include "../threading/Thread.h"

namespace Jde::Coroutine
{
	typedef uint ClientHandle;
	struct JDE_NATIVE_VISIBILITY CoWorker /*abstract*/: public IShutdown, std::enable_shared_from_this<CoWorker>
	{
		CoWorker( sv name )noexcept:_name{name}{};
		void Shutdown()noexcept override;
	protected:
		void Start()noexcept;
		virtual void Process()noexcept=0;
		const str _name;
		up<Threading::InterruptibleThread> _pThread;
		std::condition_variable _cv; mutable std::mutex _mtx;
		static sp<CoWorker> _pInstance;
		static std::once_flag _singleThread;
	private:
		void Run()noexcept;
	};

	template<typename TDerived,typename TAwaitable>
	struct JDE_NATIVE_VISIBILITY TCoWorker /*abstract*/: CoWorker
	{
		TCoWorker( sv name )noexcept:CoWorker{name}{};
		template<typename TAwaitable2=TAwaitable>
		struct Handles{ typename TAwaitable2::Handle HCoroutine; Coroutine::ClientHandle HClient; };
		virtual ~TCoWorker()noexcept{ DBG("TCoWorker::~TCoWorker({})"sv, _name); }
		static sp<TDerived> Instance()noexcept{ return static_pointer_cast<TDerived>(_pInstance); }
	protected:


//		static sp<TDerived> _pInstance; no cpp to put it in.
		static constexpr Duration WakeDuration{5s};
	};
/*	template<typename TDerived,typename TAwaitable>
	sp<TDerived> TCoWorker<TDerived,TAwaitable>::Instance()noexcept
	{
		auto pInstance = static_pointer_cast<TDerived>( _pInstance );
		//sp<TDerived> pInstance;// = _pInstance;
		if( !pInstance && !IApplication::ShuttingDown() )
		{
			std::call_once( _singleThread, [&pInstance]()mutable
			{
				pInstance = make_shared<TDerived>();
				pInstance->Start();
			});
		}
		return pInstance;
	}*/
}