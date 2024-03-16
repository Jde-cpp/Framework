#pragma once
#ifndef COROUTINE_H
#define COROUTINE_H
#include <list>
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
		ResumeThread( str Name, Duration idleLimit, CoroutineParam&& param )ι;
		~ResumeThread();
		α Resume( CoroutineParam&& param )ι->optional<CoroutineParam>;
		α Done()Ι{ return _thread.get_stop_token().stop_requested(); }
	private:
		const Duration IdleLimit;
		const Threading::ThreadParam ThreadParam;
		optional<CoroutineParam> _param; mutex _paramMutex;
		jthread _thread;
	};

	struct Γ CoroutinePool final: IShutdown
	{
		Ω Resume( coroutine_handle<>&& h )ι->void;
		α Shutdown()ι->void;
#define SETTINGS(T,n,dflt) optional<T> v; if( _pSettings ) v=_pSettings->TryGet<T>(n); return v.value_or(dflt)
	private:
		α InnerResume( CoroutineParam&& param )ι->void;
		optional<CoroutineParam> StartThread( CoroutineParam&& param )ι;
		α Run()ι->void;

		static std::shared_mutex _mtx;
		std::list<ResumeThread> _threads;
		up<Threading::InterruptibleThread> _pThread;
		up<QueueMove<CoroutineParam>> _pQueue;
		static sp<CoroutinePool> _pInstance;

		static Settings::Item<uint16> MaxThreadCount;
		static Settings::Item<Duration> WakeDuration;
		static Settings::Item<Duration> ThreadDuration;
		static Settings::Item<Duration> PoolIdleThreshold;
		//Ω MaxThreadCount()ι->uint16{ SETTINGS(uint16, "maxThreadCount", 100); }//max number of threads pool can hold
		//Ω WakeDuration()ι->Duration{ return Settings::Global().TryGet<Duration>("wakeDuration").value_or(5s); };//wake up to check for shutdown
		//Ω ThreadDuration()ι->Duration{ SETTINGS(Duration, "threadDuration", 1s); }//keep alive after buffer queue empties.
		//Ω PoolIdleThreshold()ι->Duration{ SETTINGS(Duration, "poolIdleThreshold", 1s); }//keep alive for idle pool members.

		static constexpr sv Name{ "CoroutinePool"sv };
		static sp<Settings::Container> _pSettings;
		//static ELogLevel _level;
		friend CoroutineTests;
	};
}
#endif