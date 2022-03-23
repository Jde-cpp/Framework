#pragma once
//#include <jde/Exports.h>
#include <jde/coroutine/Task.h>
#include "Coroutine.h"
namespace Jde
{
	using namespace Jde::Coroutine;
}

namespace Jde::Coroutine
{
	using ClientHandle = uint;
	/*https://stackoverflow.com/questions/44960760/msvc-dll-exporting-class-that-inherits-from-template-cause-lnk2005-already-defin
	template<class TTask=Task>
	struct TAwait
	{
		using TResult=typename TTask::TResult;
		using TPromise=typename TTask::promise_type;
		using THandle=coroutine_handle<TPromise>;
		TAwait()ι=default;
		TAwait( string name )ι:_name{move(name)}{};

		β await_ready()ι->bool{ return false; }
		β await_resume()ι->TResult=0;
		β await_suspend( coroutine_handle<typename TTask::promise_type> / *h* / )ι->void{ OriginalThreadParamPtr = { Threading::GetThreadDescription(), Threading::GetAppThreadHandle() }; }
		α AwaitResume()ι->void
		{
			if( _name.size() )
				DBG("({}){}::await_resume"sv, std::this_thread::get_id(), _name);
			if( OriginalThreadParamPtr )
				Threading::SetThreadInfo( *OriginalThreadParamPtr );
		}
	protected:
		optional<Threading::ThreadParam> OriginalThreadParamPtr;
		uint ThreadHandle;
		string ThreadName;
		const string _name;
	};
	template struct TAwait<Task>;
	*/
	struct Await
	{
		Await()ι=default;
		Await( string name )ι:_name{move(name)}{};

		β await_ready()ι->bool{ return false; }
		β await_resume()ι->AwaitResult=0;
		β await_suspend( HCoroutine )ι->void=0;//->void{ OriginalThreadParamPtr = { Threading::GetThreadDescription(), Threading::GetAppThreadHandle() }; }
		α AwaitSuspend()ι{ OriginalThreadParamPtr = { Threading::GetThreadDescription(), Threading::GetAppThreadHandle() }; }
		α AwaitResume()ι->void
		{
//			if( _name.size() )
//				DBG("({}){}::await_resume"sv, std::this_thread::get_id(), _name);
			if( OriginalThreadParamPtr )
				Threading::SetThreadInfo( *OriginalThreadParamPtr );
		}
	protected:
		optional<Threading::ThreadParam> OriginalThreadParamPtr;
		const string _name;
	};
#define Base Await
	struct IAwait : Base
	{
		IAwait( SRCE )ι:_sl{sl}{};
		IAwait( string name, SRCE )ι:Base{move(name)},_sl{sl}{};
		virtual ~IAwait()=0;
		α await_suspend( HCoroutine h )ι->void override
		{
			Base::AwaitSuspend();
			_pPromise = &h.promise();
			_pPromise->SetUnhandledResume( h );
			if( auto& ro = _pPromise->get_return_object(); ro.HasResult() )
				ro.Clear();
		}
		α await_resume()ι->AwaitResult override{ AwaitResume(); ASSERT(_pPromise); return move(_pPromise->get_return_object().Result()); }
		//α Set( AwaitResult::Value&& x )->void{ ASSERT( _pPromise ); _pPromise->get_return_object().SetResult( move(x) ); }
		ⓣ Set( up<T>&& p )->void{ ASSERT( _pPromise ); _pPromise->get_return_object().SetResult<T>( move(p) ); }
		α SetException( up<IException> p )->void{ ASSERT( _pPromise ); _pPromise->get_return_object().SetResult( move(*p) ); }
		//α Get()ι(false)->sp<void>{ ASSERT( _pPromise ); return _pPromise->get_return_object().Get( _sl ); }

		source_location _sl;
	protected:
		Task::promise_type* _pPromise{ nullptr };
		friend Task;

	};
	inline IAwait::~IAwait(){}

	//run synchronous function in coroutine pool thread.
	Τ struct TPoolAwait final : IAwait//todo rename FromSyncAwait?
	{
		using base=IAwait;
		TPoolAwait( function<up<T>()> fnctn )ι:
			_fnctn{fnctn}
		{};
		α await_suspend( HCoroutine h )ι->void override{ base::await_suspend(h); CoroutinePool::Resume( move(h) ); }

		α await_resume()ι->AwaitResult override;
	private:
		function<up<T>()> _fnctn;
	};
	struct PoolAwait final : IAwait//todo rename FromSyncAwait?
	{
		using base=IAwait;
		PoolAwait( function<void()> fnctn, SRCE )ι:base{sl},_fnctn{fnctn}{};
		α await_suspend( HCoroutine h )ι->void override{ base::await_suspend(h); CoroutinePool::Resume( move(h) ); }
		α await_resume()ι->AwaitResult override
		{
			AwaitResume();
			AwaitResult y{};
			try{ _fnctn(); }
			catch( IException& e )
			{
				y.Set( move(*e.Move()) );
			}
			return y;
		}
	private:
		function<void()> _fnctn;
	};

/*	template<class T>
	struct Task : std::promise<T>
	{
		~Task(){}
	};*/

	ⓣ CallAwait( std::promise<up<T>>&& p_, IAwait&& a )->Task
	{
		auto p = move( p_ );
		AwaitResult r = co_await a;
		ASSERTSL( !r.HasShared(), a._sl );
		if( r.HasValue() )
			p.set_value( r.UP<T>() );
		else if( r.HasError() )
			p.set_exception( r.Error()->Ptr() );
	}

	ⓣ Future( IAwait&& a )->std::future<up<T>>
	{
		auto p = std::promise<up<T>>();
		std::future<up<T>> f = p.get_future();
		CallAwait( move(p), move(a) );
		return f;
	}

	Ξ BCallAwait( std::promise<bool>&& p_, IAwait&& a )->Task
	{
		auto p = move( p_ );
		AwaitResult r = co_await a;
		if( r.HasBool() )
			p.set_value( r.Bool() );
		else if( r.HasError() )
			p.set_exception( r.Error()->Ptr() );
		else
			p.set_exception( Exception{ a._sl, "no bool.  HasValue={}, HasShared={}", r.HasValue(), r.HasShared() }.Ptr() );
	}

	Ξ BFuture( IAwait&& a )->std::future<bool>
	{
		auto p = std::promise<bool>();
		std::future<bool> f = p.get_future();
		BCallAwait( move(p), move(a) );
		return f;
	}

	Ξ CallVAwait( std::promise<void>&& p_, IAwait&& a )->Task
	{
		auto p = move( p_ );
		AwaitResult r = co_await a;
		if( r.HasError() )
			p.set_exception( r.Error()->Ptr() );
		else
			p.set_value();
	}

	Ξ VFuture( IAwait&& a )->std::future<void>
	{
		std::promise<void> p;
		std::future<void> f = p.get_future();
		CallVAwait( move(p), move(a) );
		return f;
	}

	ⓣ CallAwait( std::promise<sp<T>>&& p_, IAwait&& a )->Task
	{
		auto p = move( p_ );
		AwaitResult r = co_await a;
		ASSERT( !r.HasValue() )
		if( r.HasShared() )
			p.set_value( r.SP<T>(a._sl) );
		else if( r.HasError() )
		{
			up<IException> e = r.Error();
			std::exception_ptr e2 = e->Ptr();
			p.set_exception( e2 );
		}
		else
			ASSERT( false );
	}

	ⓣ SFuture( IAwait&& a )->std::future<sp<T>>
	{
		std::promise<sp<T>> p;
		std::future<sp<T>> f = p.get_future();
		CallAwait( move(p), move(a) );
		return f;
	}

	//assynchronous function continues at end
	class AsyncAwait /*final*/ : public IAwait
	{
		using base=IAwait;
	public:
		AsyncAwait( function<Task(HCoroutine)> fnctn, SRCE, str name={} )ι:base{name, sl}, _fnctn{fnctn}{};

		α await_suspend( HCoroutine h )ι->void override{ base::await_suspend(h); _fnctn( move(h) ); }
	protected:
		function<Task(HCoroutine)> _fnctn;
	};

	class AsyncReadyAwait final : public AsyncAwait
	{
		using base=AsyncAwait;
	public:
		AsyncReadyAwait( function<optional<AwaitResult>()> ready, function<Task(HCoroutine)> fnctn, SRCE, str name={} )ι:base{fnctn, sl, name}, _ready{ready}{};

		α await_ready()ι->bool override{  _result=_ready(); return _result.has_value(); }
		α await_resume()ι->AwaitResult override{ AwaitResume(); ASSERT(_result.has_value() || _pPromise); return _result ? move(*_result) : move(_pPromise->get_return_object().Result()); }
	protected:
		function<optional<AwaitResult>()> _ready;
		optional<AwaitResult> _result;
	};

	class CacheAwait final : public AsyncAwait
	{
		using base=AsyncAwait;
	public:
		CacheAwait( sp<void*> pCache, function<Task(HCoroutine)> fnctn, SRCE, str name={} )ι:base{fnctn, sl, name}, _pCache{pCache}{}
		α await_ready()ι->bool override{ return _pCache.get(); }
		α await_suspend( HCoroutine h )ι->void override{ base::await_suspend( h ); _fnctn( move(h) ); }
		α await_resume()ι->AwaitResult override{ return _pCache ? AwaitResult{_pCache} : base::await_resume(); }
	private:
		sp<void*> _pCache;
	};
/*struct AWrapper final : IAwait
	{
		using base=IAwait;
		AWrapper( function<Task(HCoroutine h)> fnctn, string name={} )ι:base{move(name)}, _fnctn{fnctn}{ / *DBG("({})AWrapper(0x{:x})", _name, (uint)this);* / };
		~AWrapper(){ / *DBG("({})~AWrapper(0x{:x})", _name, (uint)this);* / }

		α await_suspend( HCoroutine h )ι->void override
		{
			base::await_suspend( h );
			_fnctn( move(h) );
		}
	private:
		function<Task(HCoroutine h)> _fnctn;
	};
*/
	//template<class T>
	struct CancelAwait /*abstract*/ : Await
	{
		CancelAwait( ClientHandle& handle )ι:_hClient{ NextHandle() }{ handle = _hClient; }
		virtual ~CancelAwait()=0;
	protected:
		const ClientHandle _hClient;
	};
	/*template<class T>*/ inline CancelAwait::~CancelAwait() {}

	ⓣ TPoolAwait<T>::await_resume()ι->AwaitResult
	{
		AwaitResume();
		try
		{
			return AwaitResult{ _fnctn().release() };
		}
		catch( IException& e )
		{
			return AwaitResult{ e.Move() };
		}
	}
}
#undef Base