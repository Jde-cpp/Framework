#pragma once
#include <experimental/coroutine>
#include "../threading/Worker.h"
#include "Coroutine.h"
#include "Awaitable.h"
#include "Task.h"
#include <boost/container/flat_map.hpp>
/*#include <functional>
#include <mutex>
#include <thread>
#include "../TypeDefs.h"
*/
namespace Jde::Threading
{
	using boost::container::flat_multimap;
	using std::experimental::coroutine_handle;
	using std::experimental::suspend_never;

	struct JDE_NATIVE_VISIBILITY AlarmAwaitable final : Coroutine::CancelAwaitable<Coroutine::TaskVoid>
	{
		//typedef coroutine_handle<Coroutine::TaskVoid::promise_type> Handle;
		AlarmAwaitable( TimePoint& alarm, Coroutine::Handle& handle )noexcept:CancelAwaitable{handle}, _alarm{alarm}{}
		~AlarmAwaitable(){ /*DBG("({})AlarmAwaitable::~Awaitable"sv, std::this_thread::get_id());*/ }
		bool await_ready()noexcept{ return _alarm<Clock::now(); }
		void await_suspend( AlarmAwaitable::Handle h )noexcept; //if( !await_ready){ await_suspend();} await_resume()
		void await_resume()noexcept{ DBG("({})AlarmAwaitable::await_resume"sv, std::this_thread::get_id()); }//returns the result value for co_await expression.
	private:
		TimePoint _alarm;
	};
	struct JDE_NATIVE_VISIBILITY Alarm final: Threading::Worker
	{
		Alarm():Worker{"Alarm"}{};
		~Alarm(){ DBG0("Alarm::~Alarm"sv); }
		static auto Wait( TimePoint t, Coroutine::Handle& handle )noexcept{return AlarmAwaitable{t, handle};}
		static void Cancel( Coroutine::Handle handle )noexcept;
	private:
		void Shutdown()noexcept override;
		void Process()noexcept override;
		static sp<Alarm> Instance()noexcept;
		optional<TimePoint> Next()noexcept;
		static void Add( TimePoint t, AlarmAwaitable::Handle h, Coroutine::Handle myHandle )noexcept;
		void Add2( TimePoint t, AlarmAwaitable::Handle h, Coroutine::Handle myHandle )noexcept;
		void Cancel2( Coroutine::Handle handle )noexcept;

		std::condition_variable _cv; mutable std::mutex _mtx;
		flat_multimap<TimePoint,tuple<Coroutine::Handle,AlarmAwaitable::Handle>> _coroutines; mutex _coroutineMutex;
		static constexpr Duration WakeDuration{5s};
		friend AlarmAwaitable;
	};
}