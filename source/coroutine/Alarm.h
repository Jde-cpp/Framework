#pragma once
#include <jde/Exports.h>
#include <jde/coroutine/Task.h>
#include "../threading/Worker.h"
#include "Coroutine.h"
#include "Awaitable.h"
#include <version>

namespace Jde::Threading
{
	using boost::container::flat_multimap;
	using namespace Coroutine;
	namespace detail{ α AlarmLogTag()ι->sp<Jde::LogTag>; }

#define _logTag detail::AlarmLogTag()
	struct Γ AlarmAwait final : CancelAwait
	{
		AlarmAwait( TimePoint when, Handle& handle )ι:CancelAwait{handle}, _alarm{when}{}
		AlarmAwait( TimePoint when )ι:_alarm{when}{}
		bool await_ready()ι override{ return _alarm<Clock::now(); }
		void await_suspend( coroutine_handle<Task::promise_type> h )ι override;
		α await_resume()ι->AwaitResult override{ TRACE("({})AlarmAwait::await_resume", Threading::GetThreadId()); return {}; }//returns the result value for co_await expression.
	private:
		TimePoint _alarm;
	};

	struct Γ Alarm final: Threading::TWorker<Alarm>
	{
		using base=Threading::TWorker<Alarm>;
		Alarm():base{}{};
		~Alarm(){  TRACE("Alarm::~Alarm"sv); }
		Ω Wait( TimePoint t, Handle& handle )ι{ return AlarmAwait{t, handle}; }
		Ω Wait( Duration d )ι{ return AlarmAwait{Clock::now()+d}; }
		static void Cancel( Handle handle )ι;
		static constexpr sv Name{ "Alarm" };
	private:
		void Shutdown()ι override;
		optional<bool> Poll()ι override;
		static void Add( TimePoint t, HCoroutine h, Handle myHandle )ι;
		static constexpr Duration WakeDuration{5s};
		friend AlarmAwait;
	};
}
#undef _logTag