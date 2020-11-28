#pragma once
#include <experimental/coroutine>
#include "InterruptibleThread.h"
#include "../Settings.h"
#include "../collections/Queue.h"
#include "jthread.h"

namespace Jde::Coroutine
{
	typedef uint Handle;
	using std::experimental::coroutine_handle;
	using std::experimental::suspend_never;

	extern std::atomic<Coroutine::Handle> TaskHandle;
	extern std::atomic<Coroutine::Handle> TaskPromiseHandle;
	struct CoroutineTests;

	template<typename T>
	struct Task final
	{
		Task():_taskHandle{++TaskHandle}{ DBG("Task::Task({})"sv, _taskHandle); }
		struct promise_type
		{
			promise_type():_promiseHandle{++TaskPromiseHandle}
			{
				DBG( "promise_type::promise_type({})"sv, _promiseHandle );
			}
			Task<T>& get_return_object()noexcept{ return _returnObject; }
			suspend_never initial_suspend()noexcept{ return {}; }
			suspend_never final_suspend()noexcept{ return {}; }
			void return_void()noexcept{}
			void unhandled_exception()noexcept{}
			Task<T> _returnObject;
			const Coroutine::Handle _promiseHandle;
		};
		T Result;
		const Coroutine::Handle _taskHandle;
	};

	extern std::atomic<Coroutine::Handle> _handleIndex;
	template<typename TTask>
	struct JDE_NATIVE_VISIBILITY CancelAwaitable
	{
		typedef typename TTask::promise_type PromiseType;
		typedef coroutine_handle<PromiseType> Handle;

		CancelAwaitable( Coroutine::Handle& handle )noexcept:_hClient{ ++_handleIndex}{ handle = _hClient; }
		virtual ~CancelAwaitable()=default;//{ /*DBG("({})AlarmAwaitable::~Awaitable"sv, std::this_thread::get_id());*/ }
		virtual void await_suspend( Handle h )noexcept
		{
			OriginalThreadParam = {Threading::GetThreadDescription(), Threading::GetAppThreadHandle()};
			_pPromise = &h.promise();
		}
	protected:
		PromiseType* _pPromise{nullptr};
		const Coroutine::Handle _hClient;
		optional<Threading::ThreadParam> OriginalThreadParam;
		uint ThreadHandle;
		std::string ThreadName;
	private:
	};

	struct TaskVoid final
	{
		struct promise_type//must be promise_type
		{
			TaskVoid get_return_object()noexcept{ return {}; }
			suspend_never initial_suspend()noexcept{ return {}; }
			suspend_never final_suspend()noexcept{ return {}; }
			void return_void()noexcept{}
			void unhandled_exception()noexcept{}
		};
	};

	struct CoroutineParam : Threading::ThreadParam
	{
/*		CoroutineParam( coroutine_handle<>&& Handle, Threading::ThreadParam&& Info )noexcept;
		CoroutineParam( CoroutineParam&& rhs )noexcept;
		CoroutineParam& operator=( CoroutineParam&& rhs )noexcept;*/
		coroutine_handle<> CoHandle;
	};
	struct ResumeThread final
	{
		ResumeThread( const string& Name, Duration idleLimit, CoroutineParam&& param )noexcept;
		~ResumeThread();
		optional<CoroutineParam> Resume( CoroutineParam&& param )noexcept;
		bool Done()const noexcept{ return _thread.get_stop_token().stop_requested(); }
	private:
		const Duration IdleLimit;
		const Threading::ThreadParam ThreadParam;
		optional<CoroutineParam> _param; mutex _paramMutex;
		jthread _thread;
	};
	struct CoroutinePool final: IShutdown
	{
		static void Resume( coroutine_handle<>&& h, Threading::ThreadParam&& param )noexcept;
		void Shutdown()noexcept;
	private:
		void InnerResume( CoroutineParam&& param )noexcept;
		optional<CoroutineParam> StartThread( CoroutineParam&& param )noexcept;
		void Run()noexcept;

		static std::shared_mutex _mtx;
		std::list<ResumeThread> _threads;
		up<Threading::InterruptibleThread> _pThread;
		up<QueueMove<CoroutineParam>> _pQueue;
		static sp<CoroutinePool> _pInstance;

		//static Settings::Container& Settings()noexcept{ return *_pSettings; }
#define SETTINGS(T,n,dflt) optional<T> v; if( _pSettings ) v=_pSettings->Get2<T>(n); return v.value_or(dflt)
		static uint MaxThreadCount()noexcept{ SETTINGS(uint, "maxThreadCount", 100); }//max number of threads pool can hold
		static Duration WakeDuration()noexcept{ return Settings::Global().Get2<Duration>("wakeDuration").value_or(5s); };//wake up to check for shutdown
		static Duration ThreadDuration()noexcept{ SETTINGS(Duration, "threadDuration", 5s); }//keep alive after buffer queue empties.
		static Duration PoolIdleThreshold()noexcept{ SETTINGS(Duration, "poolIdleThreshold", 60s); }//keep alive for idle pool members.

		static constexpr sv Name{ "CoroutinePool"sv };
		static sp<Settings::Container> _pSettings;
		friend CoroutineTests;
	};
	//TODO unit tests
}