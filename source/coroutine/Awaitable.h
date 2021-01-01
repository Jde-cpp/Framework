#pragma once
#include "Coroutine.h"
#include "Task.h"
namespace Jde::Coroutine
{
	typedef uint ClientHandle;
//	extern std::atomic<ClientHandle> _handleIndex;

	template<typename TTask>
	struct CancelAwaitable
	{
		typedef typename TTask::promise_type TPromise;
		typedef coroutine_handle<TPromise> Handle;

		CancelAwaitable( ClientHandle& handle )noexcept:_hClient{ NextHandle() }{ handle = _hClient; }
		virtual ~CancelAwaitable()=default;//{ /*DBG("({})AlarmAwaitable::~Awaitable"sv, std::this_thread::get_id());*/ }
		virtual void await_suspend( Handle /*h*/ )noexcept
		{
			OriginalThreadParamPtr = {Threading::GetThreadDescription(), Threading::GetAppThreadHandle()};
			//_pPromise = &h.promise();
		}
		void AwaitResume()noexcept
		{
			if( OriginalThreadParamPtr )
				Threading::SetThreadInfo( *OriginalThreadParamPtr );
		}
	protected:
		//TPromise* _pPromise{nullptr};
		const ClientHandle _hClient;
		optional<Threading::ThreadParam> OriginalThreadParamPtr;
		uint ThreadHandle;
		std::string ThreadName;
	private:
	};
}