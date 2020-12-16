#pragma once
#include <experimental/coroutine>
#include <variant>


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
			void unhandled_exception()noexcept{ /*DBG0("unhandled_exception"sv);*/  }
			Task<T> _returnObject;
			const Handle _promiseHandle;
		};
		T Result;
		//std::exception_ptr ExceptionPtr;
		const Handle _taskHandle;
	};


	template<typename TTask>
	struct PromiseType /*notfinal*/
	{
		PromiseType():_promiseHandle{++TaskPromiseHandle}
		{}
		TTask& get_return_object()noexcept{ return _returnObject; }
		suspend_never initial_suspend()noexcept{ return {}; }
		suspend_never final_suspend()noexcept{ return {}; }
		void return_void()noexcept{}
		void unhandled_exception()noexcept{ /*DBG0("unhandled_exception"sv);*/ }
		TTask _returnObject;
		const Handle _promiseHandle;
	};


	template<typename T>
	struct TaskError final
	{
		typedef std::variant<T,std::exception_ptr> TResult;
		TaskError():_taskHandle{++TaskHandle}{ /*DBG("Task::Task({})"sv, _taskHandle);*/ }
		struct promise_type : PromiseType<TaskError<T>>
		{};
		TResult Result;
		const Handle _taskHandle;
	};

}