﻿#pragma once
#include "../threading/Thread.h"
#include "../Settings.h"
#include <jde/App.h>
#include "../collections/Queue.h"
#include "../threading/jthread.h"
#include "../threading/InterruptibleThread.h"

namespace Jde::Threading{ struct InterruptibleThread; }

namespace Jde::Coroutine
{
	struct CoroutineTests;

	struct CoroutineParam /*: Threading::ThreadParam*/
	{
		coroutine_handle<> CoHandle;
	};
	struct Γ ResumeThread final
	{
		ResumeThread( str Name, Duration idleLimit, CoroutineParam&& param )noexcept;
		~ResumeThread();
		optional<CoroutineParam> Resume( CoroutineParam&& param )noexcept;
		bool Done()const noexcept{ return _thread.get_stop_token().stop_requested(); }
	private:
		const Duration IdleLimit;
		const Threading::ThreadParam ThreadParam;
		optional<CoroutineParam> _param; mutex _paramMutex;
		jthread _thread;
	};

	struct Γ CoroutinePool final: IShutdown
	{
		static void Resume( coroutine_handle<>&& h )noexcept;
		void Shutdown()noexcept;

#define SETTINGS(T,n,dflt) optional<T> v; if( _pSettings ) v=_pSettings->TryGet<T>(n); return v.value_or(dflt)
	private:
		void InnerResume( CoroutineParam&& param )noexcept;
		optional<CoroutineParam> StartThread( CoroutineParam&& param )noexcept;
		void Run()noexcept;

		static std::shared_mutex _mtx;
		std::list<ResumeThread> _threads;
		up<Threading::InterruptibleThread> _pThread;
		up<QueueMove<CoroutineParam>> _pQueue;
		static sp<CoroutinePool> _pInstance;

		static uint MaxThreadCount()noexcept{ SETTINGS(uint, "maxThreadCount", 100); }//max number of threads pool can hold
		static Duration WakeDuration()noexcept{ return Settings::Global().TryGet<Duration>("wakeDuration").value_or(5s); };//wake up to check for shutdown
		static Duration ThreadDuration()noexcept{ SETTINGS(Duration, "threadDuration", 5s); }//keep alive after buffer queue empties.
		static Duration PoolIdleThreshold()noexcept{ SETTINGS(Duration, "poolIdleThreshold", 60s); }//keep alive for idle pool members.

		static constexpr sv Name{ "CoroutinePool"sv };
		static sp<Settings::Container> _pSettings;
		static ELogLevel _level;
		friend CoroutineTests;
	};
}