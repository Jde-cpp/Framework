#pragma once
#ifndef COROUTINE_H
#define COROUTINE_H
#include <list>
#include "../threading/Thread.h"
#include <jde/framework/settings.h>
#include <jde/framework/process.h>
#include "../collections/Queue.h"
#include "../threading/InterruptibleThread.h"

namespace Jde::Threading{ struct InterruptibleThread; }

namespace Jde::Coroutine{
	struct CoroutineTests;

	struct CoroutineParam{ /*: Threading::ThreadParam*/
		coroutine_handle<> CoHandle;
	};

	struct Γ ResumeThread final{
		ResumeThread( str Name, Duration idleLimit, CoroutineParam&& param )ι;
		~ResumeThread();
		α CheckIfReady( CoroutineParam&& param )ι->optional<CoroutineParam>;
		α Done()Ι{ return _thread.get_stop_token().stop_requested(); }
		α CanDelete()Ι{ return Done() && !_param.has_value(); }
	private:
		const Duration IdleLimit;
		const Threading::ThreadParam ThreadParam;
		optional<CoroutineParam> _param; mutex _paramMutex;
		std::jthread _thread;
	};

	struct Γ CoroutinePool final: IShutdown{
		~CoroutinePool(){ _pInstance=nullptr; }
		Ω Resume( coroutine_handle<> h )ι->void;
		α Shutdown( bool terminate )ι->void;
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

		static uint16 MaxThreadCount;
		static Duration WakeDuration;
		static Duration ThreadDuration;
		static Duration PoolIdleThreshold;

		static constexpr sv Name{ "CoroutinePool" };
		static jobject _settings;
		friend CoroutineTests;
	};
}
#endif