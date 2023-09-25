#pragma once
#include <jde/Exports.h>
#include <jde/coroutine/Task.h>
#include "../threading/Worker.h"
#include "Coroutine.h"
#include "Awaitable.h"

namespace Jde::Threading
{
	using boost::container::flat_multimap;
	using namespace Coroutine;
	struct Γ AlarmAwait final : CancelAwait
	{
		AlarmAwait( TimePoint when, Handle& handle )noexcept:CancelAwait{handle}, _alarm{when}{}
		AlarmAwait( TimePoint when )noexcept:_alarm{when}{}
		bool await_ready()noexcept override{ return _alarm<Clock::now(); }
		void await_suspend( coroutine_handle<Task::promise_type> h )noexcept override;
		α await_resume()noexcept->AwaitResult override{ TRACE("({})AlarmAwait::await_resume"sv, std::this_thread::get_id()); return {}; }//returns the result value for co_await expression.
	private:
		TimePoint _alarm;
	};

	struct Γ Alarm final: Threading::TWorker<Alarm>
	{
		using base=Threading::TWorker<Alarm>;
		Alarm():base{}{};
		~Alarm(){  DBG("Alarm::~Alarm"sv); }
		static auto Wait( TimePoint t, Handle& handle )noexcept{ return AlarmAwait{t, handle}; }
		static auto Wait( Duration d )noexcept{ return AlarmAwait{Clock::now()+d}; }
		static void Cancel( Handle handle )noexcept;
		static constexpr sv Name{ "Alarm" };
	private:
		void Shutdown()noexcept override;
		optional<bool> Poll()noexcept override;
		static void Add( TimePoint t, HCoroutine h, Handle myHandle )noexcept;
		static constexpr Duration WakeDuration{5s};
		static const LogTag& _logLevel;
		friend AlarmAwait;
	};
}