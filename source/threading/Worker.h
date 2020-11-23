#pragma once
#include "../application/Application.h"
#include "InterruptibleThread.h"

namespace Jde::Threading
{
	struct JDE_NATIVE_VISIBILITY Worker /*abstract*/: public IShutdown, std::enable_shared_from_this<Worker>
	{
		Worker( sv name )noexcept;
		virtual ~Worker()noexcept{ DBG("Worker::~Worker({})"sv, _name); }
		void Shutdown()noexcept override;
	protected:
		void Start()noexcept;
		static sp<Worker> _pInstance;
		up<Threading::InterruptibleThread> _pThread;
	private:
		void Run()noexcept;
		virtual void Process()noexcept=0;
		string _name;
	};
}