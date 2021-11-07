#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <jde/TypeDefs.h>
#include <jde/Exports.h>

namespace Jde::Threading
{
	struct InterruptibleThread;
	//derive from, sleeps till interrupted

	class Interrupt
	{
	public:
		Γ Interrupt( sv threadName, Duration duration, bool paused=false );
		Γ virtual ~Interrupt();
		Interrupt()=delete;
		Interrupt(const Interrupt &)=delete;
		Interrupt& operator=(const Interrupt &)=delete;

		//Interrupt( sv threadName, bool paused=false );
		Γ void Wake()noexcept;
		Γ void Stop()noexcept;

		bool IsPaused()const noexcept{ return _paused; }
		Γ virtual void Pause()noexcept;
		Γ virtual void UnPause()noexcept;

		virtual void OnTimeout()/*noexcept*/=0;
		virtual void OnAwake()noexcept=0;
	private:
		Γ void Start()noexcept;
		void Start2()noexcept;
		void Worker();
		std::once_flag _singleThread;
		std::condition_variable _cvWait;  std::mutex _cvMutex;
		up<Threading::InterruptibleThread> _pThread;
		const string _threadName;
		atomic<bool> _paused{ false };
		std::chrono::nanoseconds _refreshRate;
	};
}