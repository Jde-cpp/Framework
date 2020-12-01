#pragma once
#include <experimental/coroutine>

namespace Jde::Coroutine
{
	typedef uint Handle;
	extern std::atomic<Handle> TaskHandle;
	extern std::atomic<Handle> TaskPromiseHandle;
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

	template<typename T>
	struct Task final
	{
		Task():_taskHandle{++TaskHandle}{ /*DBG("Task::Task({})"sv, _taskHandle);*/ }
		struct promise_type
		{
			promise_type():_promiseHandle{++TaskPromiseHandle}
			{
				//DBG( "promise_type::promise_type({})"sv, _promiseHandle );
			}
			Task<T>& get_return_object()noexcept{ return _returnObject; }
			suspend_never initial_suspend()noexcept{ return {}; }
			suspend_never final_suspend()noexcept{ return {}; }
			void return_void()noexcept{}
			void unhandled_exception()noexcept{}
			Task<T> _returnObject;
			const Handle _promiseHandle;
		};
		T Result;
		const Handle _taskHandle;
	};

}