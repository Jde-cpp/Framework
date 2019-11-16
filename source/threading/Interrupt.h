#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include "../TypeDefs.h"

namespace Jde::Threading
{
	//derive from, sleeps till interrupted

	class Interrupt
	{
	public:
		JDE_NATIVE_VISIBILITY Interrupt( string_view threadName, Duration duration, bool paused=false );
		JDE_NATIVE_VISIBILITY virtual ~Interrupt();
		Interrupt()=delete;
		Interrupt(const Interrupt &)=delete;
		Interrupt& operator=(const Interrupt &)=delete;
		
		//Interrupt( string_view threadName, bool paused=false );
		JDE_NATIVE_VISIBILITY void Wake()noexcept;
		JDE_NATIVE_VISIBILITY void Stop()noexcept;

		bool IsPaused()const noexcept{ return _paused; }
		JDE_NATIVE_VISIBILITY virtual void Pause()noexcept;
		JDE_NATIVE_VISIBILITY virtual void UnPause()noexcept;

		virtual void OnTimeout()/*noexcept*/=0;
		virtual void OnAwake()noexcept=0;
	private:
		JDE_NATIVE_VISIBILITY void Start()noexcept;
		void Start2()noexcept;
		void Worker();
		std::once_flag _singleThread;
		std::condition_variable _cvWait;  std::mutex _cvMutex;
		up<std::thread> _pThread;
		std::atomic<bool> _continue{true};
		const string _threadName;
		std::atomic<bool> _paused{false};
		std::chrono::nanoseconds _refreshRate;
		const ELogLevel _logLevel{ELogLevel::Trace};
	};
}