#pragma once
#ifndef MUTEX_H
#define MUTEX_H
#include <shared_mutex>
#include <queue>
#include <boost/core/noncopyable.hpp>
#include <jde/framework/coroutine/Await.h>
#include <jde/framework/thread/thread.h>

#define Φ Γ auto
namespace Jde{
/*	struct AtomicGuard final : boost::noncopyable{
		AtomicGuard( std::atomic_flag& f )ι: _pValue{ &f }{
			while( f.test_and_set(std::memory_order_acquire) ){
        while( f.test(std::memory_order_relaxed) )
					std::this_thread::yield();
			}
		}
		AtomicGuard( AtomicGuard&& x )ι: _pValue{ x._pValue }{ x._pValue = nullptr; }
		~AtomicGuard(){ if( _pValue ){ ASSERT(_pValue->test(std::memory_order_relaxed)) _pValue->clear(std::memory_order_release); } }
		α unlock()ι->void{ ASSERT(_pValue) _pValue->clear( std::memory_order_release ); _pValue = nullptr; }
	private:
		atomic_flag* _pValue;
	};*/

	struct CoLock;
	struct Γ CoGuard{
		CoGuard()ι{ ASSERT(false); }
		CoGuard( CoGuard&& lock )ι;
		~CoGuard();
		α operator=( CoGuard&& )ι->CoGuard&;
		α unlock()ι->void;
	private:
		CoGuard( CoLock& lock )ι;
		CoGuard( const CoGuard& )ι = delete;
		α operator=( const CoGuard& )ι->CoGuard& = delete;
		CoLock* _lock{};
		friend CoLock; friend class LockAwait;
	};

	class Γ LockAwait : public TAwait<CoGuard>{
		using base=TAwait<CoGuard>;
	public:
		LockAwait( CoLock& lock )ι;
		~LockAwait();
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ι->CoGuard override;
	private:
		CoLock& _lock;
		optional<CoGuard> _guard;
	};

	struct Γ CoLock{
		α Lock(){ return LockAwait{*this}; }
	private:
		α TestAndSet()ι->optional<CoGuard>;
		α Push( LockAwait::Handle h )ι->optional<CoGuard>;
		α Clear()ι->void;

		std::queue<LockAwait::Handle> _queue; mutex _mutex;
		atomic_flag _locked;
		friend class LockAwait; friend struct CoGuard;
	};
}
namespace Jde::Threading{
//	Φ UniqueLock( str key )ι->std::unique_lock<std::shared_mutex>;
}
#undef Φ
#endif