#pragma once
#include <jde/framework/coroutine/Task.h>
#include "../threading/Worker.h"
#include "Coroutine.h"
#include "Awaitable.h"
#include <version>

namespace Jde::Threading{
	using boost::container::flat_multimap;
	using namespace Coroutine;

	struct Γ AlarmAwait final : CancelAwait{
		AlarmAwait( TimePoint when, Handle& handle, SL sl )ι:CancelAwait{handle}, _alarm{when}, _sl{sl}{}
		AlarmAwait( TimePoint when, SL sl )ι:_alarm{when}, _sl{sl}{}
		α await_ready()ι->bool override{ return _alarm<Clock::now(); }
		α Suspend()ι->void override;
		α await_resume()ι->AwaitResult override{ Trace(ELogTags::Scheduler, "({})AlarmAwait::await_resume", Threading::GetThreadId()); return {}; }//returns the result value for co_await expression.
	private:
		TimePoint _alarm;
		SL _sl;
	};

	struct Γ Alarm final: Threading::TWorker<Alarm>{
		using base=Threading::TWorker<Alarm>;
		Alarm():base{}{};
		Ω Wait( TimePoint t, Handle& handle, SRCE )ι{ return AlarmAwait{t, handle, sl}; }
		Ω Wait( Duration d, SRCE )ι{ return AlarmAwait{Clock::now()+d, sl}; }
		static void Cancel( Handle handle )ι;
		static constexpr sv Name{ "alarm" };
	private:
		void Shutdown( bool terminate )ι override;
		optional<bool> Poll()ι override;
		static void Add( TimePoint t, HCoroutine h, Handle myHandle, SL sl )ι;
		static constexpr Duration WakeDuration{5s};
		friend AlarmAwait;
	};
}