#pragma once
#include "boost/noncopyable.hpp"
#include <jde/framework/coroutine/Await.h>
#include "Awaitable.h"



namespace Jde{
	using namespace Coroutine;
	struct LockKeyAwait;
	struct Γ CoLockGuard final{
		CoLockGuard( string Key, std::variant<LockKeyAwait*,coroutine_handle<>> )ι;
		CoLockGuard( CoLockGuard&& rhs )ι;
		α operator=( CoLockGuard&& rhs )ι->CoLockGuard&;
		~CoLockGuard();
	private:
		variant<LockKeyAwait*,coroutine_handle<>> Handle;
		string Key;
	};

	struct Γ LockKeyAwait final : TAwait<CoLockGuard>, boost::noncopyable{
		using base=TAwait<CoLockGuard>;
		LockKeyAwait( string key )ι:Key{move(key)}{}

		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ι->CoLockGuard override;
	private:
		base::Handle Handle{nullptr};
		const string Key;
	};

	using namespace Coroutine;
	/*
	struct Γ LockWrapperAwait final: IAwaitOld{
		LockWrapperAwait( string key, function<void(Coroutine::AwaitResult&)> f, bool shared=true, SRCE ):IAwaitOld{sl},_key{move(key)}, _f{f}, _shared{shared}{}//TODO implement shared
		LockWrapperAwait( string key, function<void(Coroutine::AwaitResult&, up<CoLockGuard> l)> f, bool shared=true, SRCE ):IAwaitOld{sl},_key{move(key)}, _f{f}, _shared{shared}{}//TODO implement shared
		Ω TryLock( string key, bool shared )ι->up<CoLockGuard>;
		α await_ready()ι->bool override;
		α AwaitSuspend()ι->Task;
		α Suspend()ι->void override;
		α await_resume()ι->AwaitResult override;
		α ReadyResult()ι->sp<AwaitResult>{ ASSERT(!_pReadyResult); _pReadyResult = ms<AwaitResult>(); return _pReadyResult; }
	private:
		const string _key;
		sp<AwaitResult> _pReadyResult;
		std::variant<function<void(AwaitResult&)>,function<void(Coroutine::AwaitResult&, up<CoLockGuard> l)>> _f;
		const bool _shared;
		up<CoLockGuard> _pLock;
	};
	*/
}