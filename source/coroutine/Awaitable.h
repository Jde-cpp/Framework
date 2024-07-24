#pragma once
#ifndef AWAITABLE_H
#define AWAITABLE_H
#include <jde/coroutine/Task.h>
#include "Coroutine.h"
namespace Jde{
	using namespace Jde::Coroutine;
}

namespace Jde::Coroutine{
	using ClientHandle = uint;
	template<class TResult=void,class TTask=VoidTask>
	struct VoidAwait{
	protected:
	public:
		using TPromise = TTask::promise_type;
		using Task=TTask;
		using Handle=coroutine_handle<TPromise>;
		VoidAwait( SRCE )ι:_sl{sl}{}
		β await_ready()ι->bool{ return false; }
		β await_suspend( Handle h )ε->void{ _h=h; }
		β await_resume()ε->TResult{ AwaitResume(); return TResult{}; }
		α Resume()ι{ ASSERT(_h); _h->resume(); }
	protected:
		α AwaitResume()ε->void{
			if( up<IException> e = Promise() ? Promise()->MoveError() : nullptr; e )
				e->Throw();
		}
		Handle _h{};
		TPromise* Promise(){ return _h ? &_h.promise() : nullptr; }
		SL _sl;
	};

	template<class Result,class TTask=Coroutine::TTask<Result>>
	struct TAwait : VoidAwait<Result,TTask>{
		using base = VoidAwait<Result,TTask>;
		TAwait( SRCE )ι:base{sl}{}
		β await_resume()ε->Result{
			base::AwaitResume();
			if( !base::Promise() )
				throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Critical, "promise is null" };
			if( !base::Promise()->Value() )
				throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Critical, "Value is null" };
			return move( *base::Promise()->Value() );
		};
		α Resume( Result&& r )ι{ ASSERT(base::Promise()); base::Promise()->Resume( move(r), base::_h ); }
		α Resume( IException&& r )ι{ ASSERT(base::Promise()); base::Promise()->ResumeWithError( move(r), base::_h ); }
	};
//TODO look into combining BaseAwait and IAwait
	struct BaseAwait{
		BaseAwait()ι=default;
		BaseAwait( str name )ι:_name{name}{};

		β await_ready()ι->bool{ return false; }
		β await_resume()ι->AwaitResult=0;
		β await_suspend( HCoroutine )ι->void=0;//->void{ OriginalThreadParamPtr = { Threading::GetThreadDescription(), Threading::GetAppThreadHandle() }; }
		α AwaitSuspend()ι{ OriginalThreadParamPtr = { Threading::GetThreadDescription(), Threading::GetAppThreadHandle() }; }
		α AwaitResume()ι->void{
			if( OriginalThreadParamPtr )
				Threading::SetThreadInfo( *OriginalThreadParamPtr );
		}
	protected:
		optional<Threading::ThreadParam> OriginalThreadParamPtr;
		const string _name;
		const source_location _sl;
	};

	class IAwait : public BaseAwait{
		using base=BaseAwait;
	public:
		IAwait( SRCE )ι:_sl{sl}{};
		IAwait( str name, SRCE )ι:base{name},_sl{sl}{};
		virtual ~IAwait()=0;

	//	Ŧ await_transform(T&& awaitable)ι->T&&{ return static_cast<T&&>(awaitable); }

		α await_suspend( HCoroutine h )ι->void override{
			base::AwaitSuspend();
			_pPromise = &h.promise();
			//_pPromise->SetUnhandledResume( h );
			//if( _pPromise->HasResult() )
			//	ro.Clear();
		}
		α await_resume()ι->AwaitResult override{
			ASSERT( _pPromise );
			AwaitResume();
			//auto& y = _pPromise->get_return_object();
			_pPromise->Push( _sl );
			return _pPromise->MoveResult();
		}
		Ŧ Set( up<T>&& p )->void{ ASSERT( _pPromise ); _pPromise->SetResult<T>( move(p) ); }
		α SetException( up<IException> p )->void{ ASSERT( _pPromise ); _pPromise->SetResult( move(*p) ); }

		const source_location _sl;
	protected:
		Task::promise_type* _pPromise{};
		friend Task;
	};
	inline IAwait::~IAwait(){}

	//Holds a result variable for await_ready
	struct IAwaitCache : IAwait{
		IAwaitCache( SRCE )ι:IAwait{sl}{};
		IAwaitCache( string name, SRCE )ι:IAwait{move(name), sl}{};
		//virtual ~IAwait()=0;
		α await_suspend( HCoroutine h )ι->void override{ IAwait::await_suspend(h); }
		α await_resume()ι->AwaitResult override{
			if(_pPromise)
				_result = IAwait::await_resume();
			else{
				AwaitResume();
				_result.Push(_sl);
			}
			return move(_result);
		}
	protected:
		AwaitResult _result;
		friend Task;
	};

	//run synchronous function in coroutine pool thread.
	Τ struct TPoolAwait final : IAwait{//todo rename FromSyncAwait?
		using base=IAwait;
		TPoolAwait( function<up<T>()> fnctn, SRCE )ι:base{sl},_fnctn{fnctn}{};
		TPoolAwait( function<up<T>()> fnctn, str threadName, SRCE )ι:base{threadName, sl},_fnctn{fnctn}{};
		α await_suspend( HCoroutine h )ι->void override{
			base::await_suspend(h);
			if( _name.size() )
				OriginalThreadParamPtr = { _name, Threading::GetAppThreadHandle() };
			CoroutinePool::Resume( move(h) );
		}

		α await_resume()ι->AwaitResult override;
	private:
		function<up<T>()> _fnctn;
	};
	//run synchronous function returning void in coroutine pool thread.
	struct PoolAwait final : IAwait{//todo rename FromSyncAwait?
		using base=IAwait;
		PoolAwait( function<void()> fnctn, SRCE )ι:base{sl},_fnctn{fnctn}{};
		α await_suspend( HCoroutine h )ι->void override{ base::await_suspend(h); CoroutinePool::Resume( move(h) ); }
		α await_resume()ι->AwaitResult override{
			AwaitResume();
			AwaitResult y{};
			try{ _fnctn(); }
			catch( IException& e ){
				y.Set( move(*e.Move()) );
			}
			return y;
		}
	private:
		function<void()> _fnctn;
	};

	Ŧ CallAwait( std::promise<up<T>> p, IAwait&& a )->Task{
		//auto p = move( p_ );
		AwaitResult r = co_await a;
		ASSERTSL( !r.HasShared() && !r.HasBool(), a._sl );
		if( r.HasError() )
			p.set_exception( r.Error()->Ptr() );
		else
			p.set_value( r.UP<T>() );
	}

	Ŧ Future( IAwait&& a )->std::future<up<T>>{
		auto p = std::promise<up<T>>();
		std::future<up<T>> f = p.get_future();
		CallAwait( move(p), move(a) );
		return f;
	}

	Ξ BCallAwait( std::promise<bool>&& p_, IAwait&& a )->Task{
		auto p = move( p_ );
		AwaitResult r = co_await a;
		if( r.HasBool() )
			p.set_value( r.Bool() );
		else if( r.HasError() )
			p.set_exception( r.Error()->Ptr() );
		else
			p.set_exception( Exception{ a._sl, "no bool.  HasValue={}, HasShared={}", r.HasValue(), r.HasShared() }.Ptr() );
	}

	Ξ BFuture( IAwait&& a )->std::future<bool>{
		auto p = std::promise<bool>();
		std::future<bool> f = p.get_future();
		BCallAwait( move(p), move(a) );
		return f;
	}

	Ξ CallVAwait( std::promise<void>&& p_, IAwait&& a )->Task{
		auto p = move( p_ );
		AwaitResult r = co_await a;
		if( r.HasError() )
			p.set_exception( r.Error()->Ptr() );
		else
			p.set_value();
	}

	Ξ VFuture( IAwait&& a )->std::future<void>{
		std::promise<void> p;
		std::future<void> f = p.get_future();
		CallVAwait( move(p), move(a) );
		return f;
	}

	Ŧ CallAwait( std::promise<sp<T>> p, IAwait&& a )ι->Task{
		AwaitResult r = co_await a;
		ASSERT( !r.HasValue() )
		if( r.HasShared() )
			p.set_value( r.SP<T>(a._sl) );
		else if( r.HasError() ){
			up<IException> e = r.Error();
			std::exception_ptr e2 = e->Ptr();
			p.set_exception( e2 );
		}
		else
			ASSERT( false );
	}

	Ŧ SFuture( IAwait&& a )ι->std::future<sp<T>>{
		std::promise<sp<T>> p;
		std::future<sp<T>> f = p.get_future();
		CallAwait( move(p), move(a) );
		return f;
	}

#define await(t,a) *( co_await a ).UP<t>()
#define awaitp(t,a) ( co_await a ).UP<t>()

	//await_suspend is async.
	class AsyncAwait /*final*/ : public IAwait{
		using base=IAwait;
	public:
		AsyncAwait( function<void(HCoroutine)>&& suspend, SRCE, str name={} )ι:base{name, sl}, _suspend{move(suspend)}{};

		α await_suspend( HCoroutine h )ι->void override{ base::await_suspend(h); _suspend( move(h) ); }
	protected:
		function<void(HCoroutine)> _suspend;
	};

	class AsyncReadyAwait : public AsyncAwait{
		using base=AsyncAwait;
	public:
		using ReadyFunc=function<optional<AwaitResult>()>;
		//AsyncReadyAwait()ι:base{ [](HCoroutine){return Task{};} },_ready{ [](){return AwaitResult{};} }{}
		AsyncReadyAwait( ReadyFunc&& ready, function<void(HCoroutine)>&& suspend, SRCE, str name={} )ι:base{move(suspend), sl, name}, _ready{move(ready)}{};

		α await_ready()ι->bool override{  _result=_ready(); return _result.has_value(); }
		α await_resume()ι->AwaitResult override{ AwaitResume(); ASSERT(_result.has_value() || _pPromise); return _result ? move(*_result) : _pPromise->MoveResult(); }
	protected:
		ReadyFunc _ready;
		optional<AwaitResult> _result;
	};

	class CacheAwait final : public AsyncAwait{
		using base=AsyncAwait;
	public:
		CacheAwait( sp<void*> pCache, function<Task(HCoroutine)> fnctn, SRCE, str name={} )ι:base{fnctn, sl, name}, _pCache{pCache}{}
		α await_ready()ι->bool override{ return _pCache.get(); }
		α await_suspend( HCoroutine h )ι->void override{ base::await_suspend( h ); _suspend( move(h) ); }
		α await_resume()ι->AwaitResult override{ return _pCache ? AwaitResult{move(_pCache)} : base::await_resume(); }
	private:
		sp<void> _pCache;
	};

	// Allows cancellation of an awaitable.  Used for alarms.
	struct CancelAwait /*abstract*/ : BaseAwait{
		CancelAwait()ι:_hClient{ NextHandle() }{}
		CancelAwait( ClientHandle& handle )ι:_hClient{ NextHandle() }{ handle = _hClient; }
		virtual ~CancelAwait()=0;
	protected:
		const ClientHandle _hClient;
	};
	inline CancelAwait::~CancelAwait() {}

	Ŧ TPoolAwait<T>::await_resume()ι->AwaitResult{
		AwaitResume();
		AwaitResult y;
		try{
			Threading::SetThreadDscrptn( _name );
			up<T> p = _fnctn();
			y.Set( p.release() );
		}
		catch( IException& e ){
			y.Set( move(e) );
		}
		catch( ... ){ //from future
			auto p = std::current_exception();
			if( p )
				y.Set( move(*IException::FromExceptionPtr(p, _sl)) );
			else
				y.Set( Exception{ _sl, ELogLevel::Critical, "unknown" } );
		}
		return y;
	}
}
#undef Base
#endif