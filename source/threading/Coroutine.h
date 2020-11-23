#pragma once
#include <experimental/coroutine>
//#include "Thread.h"
//#include "types/Proc.h"

namespace Jde::Coroutine
{
	typedef uint Handle;
	using std::experimental::coroutine_handle;
	using std::experimental::suspend_never;

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
}