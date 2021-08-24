#pragma once
//#include <boost/container/flat_map.hpp>
#include <jde/Exports.h>
#include <jde/coroutine/Task.h>
#include "../threading/Worker.h"
#include "Coroutine.h"
#include "Awaitable.h"

namespace Jde::Threading
{
	using boost::container::flat_multimap;
	using namespace Coroutine;
	struct JDE_NATIVE_VISIBILITY AlarmAwaitable final : CancelAwaitable<Task2>
	{
		AlarmAwaitable( TimePoint& alarm, Handle& handle )noexcept:CancelAwaitable{handle}, _alarm{alarm}{}
		//~AlarmAwaitable(){ /*DBG("({})AlarmAwaitable::~Awaitable"sv, std::this_thread::get_id());*/ }
		bool await_ready()noexcept override{ return _alarm<Clock::now(); }
		void await_suspend( coroutine_handle<Task2::promise_type> h )noexcept override;
		TaskResult await_resume()noexcept override{ DBG("({})AlarmAwaitable::await_resume"sv, std::this_thread::get_id()); return {}; }//returns the result value for co_await expression.
	private:
		TimePoint _alarm;
	};

	struct JDE_NATIVE_VISIBILITY Alarm final: Threading::TWorker<Alarm>
	{
		using base=Threading::TWorker<Alarm>;
		Alarm():base{"Alarm"sv}{};
		~Alarm(){ if( GetDefaultLogger() ) DBG("Alarm::~Alarm"sv); }
		static auto Wait( TimePoint t, Handle& handle )noexcept{ return AlarmAwaitable{t, handle}; }
		static void Cancel( Handle handle )noexcept;
	private:
		void Shutdown()noexcept override;
		optional<bool> Poll()noexcept override;
		optional<TimePoint> Next()noexcept;
		static void Add( TimePoint t, AlarmAwaitable::THandle h, Handle myHandle )noexcept;
		void Add2( TimePoint t, AlarmAwaitable::THandle h, Handle myHandle )noexcept;
		void Cancel2( Handle handle )noexcept;

		std::condition_variable _cv; mutable std::mutex _mtx;
		flat_multimap<TimePoint,tuple<Handle,AlarmAwaitable::THandle>> _coroutines; mutex _coroutineMutex;
		static constexpr Duration WakeDuration{5s};
		friend AlarmAwaitable;
	};
}