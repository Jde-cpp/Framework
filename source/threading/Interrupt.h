#pragma once
#ifndef INTERRUPT_H
#define INTERRUPT_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace Jde::Threading{
	struct InterruptibleThread;
	//derive from, sleeps till interrupted

	struct Interrupt{
		Γ Interrupt( sv threadName, Duration duration, bool paused=false );
		Γ virtual ~Interrupt();
		Interrupt()=delete;
		Interrupt(const Interrupt &)=delete;
		Interrupt& operator=(const Interrupt &)=delete;

		//Interrupt( sv threadName, bool paused=false );
		Γ void Wake()ι;
		Γ void Stop()ι;

		bool IsPaused()Ι{ return _paused; }
		Γ virtual void Pause()ι;
		Γ virtual void UnPause()ι;

		virtual void OnTimeout()/*ι*/=0;
		virtual void OnAwake()ι=0;
	private:
		Γ void Start()ι;
		void Start2()ι;
		void Worker();
		std::once_flag _singleThread;
		std::condition_variable _cvWait;  std::mutex _cvMutex;
		up<Threading::InterruptibleThread> _pThread;
		const string _threadName;
		atomic<bool> _paused{ false };
		std::chrono::nanoseconds _refreshRate;
	};
}
#endif