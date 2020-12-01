#pragma once
#include <experimental/coroutine>

namespace Jde::Coroutine
{
	using std::experimental::coroutine_handle;
	using std::experimental::suspend_never;

	typedef uint ClientHandle;
	extern std::atomic<ClientHandle> _handleIndex;

	template<typename TTask>
	struct JDE_NATIVE_VISIBILITY CancelAwaitable
	{
		typedef typename TTask::promise_type PromiseType;
		typedef coroutine_handle<PromiseType> Handle;

		CancelAwaitable( ClientHandle& handle )noexcept:_hClient{ ++_handleIndex}{ handle = _hClient; }
		virtual ~CancelAwaitable()=default;//{ /*DBG("({})AlarmAwaitable::~Awaitable"sv, std::this_thread::get_id());*/ }
		virtual void await_suspend( Handle h )noexcept
		{
			OriginalThreadParam = {Threading::GetThreadDescription(), Threading::GetAppThreadHandle()};
			_pPromise = &h.promise();
		}
	protected:
		PromiseType* _pPromise{nullptr};
		const ClientHandle _hClient;
		optional<Threading::ThreadParam> OriginalThreadParam;
		uint ThreadHandle;
		std::string ThreadName;
	private:
	};
}