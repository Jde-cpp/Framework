﻿#pragma once
#include "Awaitable.h"
#include "boost/noncopyable.hpp"

namespace Jde
{
	using namespace Coroutine;
	struct Γ LockKeyAwait final : BaseAwait{
		LockKeyAwait( string key, bool shared )ι:Key{move(key)}/*, _shared{shared}*/{}
		//~LockKeyAwait(){ TRACE( "~LockKeyAwait" ); }

		α await_ready()ι->bool override;
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override;
	private:
		HCoroutine Handle{nullptr};
		const string Key;
		//const bool _shared;//TODO implement
	};
	Ξ CoLockKey( string key, bool shared )ι{ return LockKeyAwait{move(key), shared}; }

	struct CoLockGuard final : boost::noncopyable{
		CoLockGuard( string Key, std::variant<LockKeyAwait*,coroutine_handle<>> )ι;
		Γ ~CoLockGuard();
	private:
		std::variant<LockKeyAwait*,coroutine_handle<>> Handle;
		const string Key;
	};

	using namespace Coroutine;
	struct Γ LockWrapperAwait final: IAwait{
		LockWrapperAwait( string key, function<void(Coroutine::AwaitResult&)> f, bool shared=true, SRCE ):IAwait{sl},_key{move(key)}, _f{f}, _shared{shared}{}//TODO implement shared
		LockWrapperAwait( string key, function<void(Coroutine::AwaitResult&, up<CoLockGuard> l)> f, bool shared=true, SRCE ):IAwait{sl},_key{move(key)}, _f{f}, _shared{shared}{}//TODO implement shared
		Ω TryLock( string key, bool shared )ι->up<CoLockGuard>;
		α await_ready()ι->bool override;
		α AwaitSuspend( HCoroutine h )->Task;
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override;
		α ReadyResult()ι->sp<AwaitResult>{ ASSERT(!_pReadyResult); _pReadyResult = ms<AwaitResult>(); return _pReadyResult; }
	private:
		const string _key;
		sp<AwaitResult> _pReadyResult;
		std::variant<function<void(AwaitResult&)>,function<void(Coroutine::AwaitResult&, up<CoLockGuard> l)>> _f;
		const bool _shared;
		up<CoLockGuard> _pLock;
	};
}