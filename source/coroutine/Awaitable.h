#pragma once
#include <jde/Exports.h>
#include <jde/coroutine/Task.h>
#include "Coroutine.h"
namespace Jde{ using namespace Jde::Coroutine; }
namespace Jde::Coroutine
{
	using ClientHandle = uint;
	using HCoroutine = coroutine_handle<Task::promise_type>;

	/*https://stackoverflow.com/questions/44960760/msvc-dll-exporting-class-that-inherits-from-template-cause-lnk2005-already-defin
	template<class TTask=Task>
	struct TAwait
	{
		using TResult=typename TTask::TResult;
		using TPromise=typename TTask::promise_type;
		using THandle=coroutine_handle<TPromise>;
		TAwait()noexcept=default;
		TAwait( string name )noexcept:_name{move(name)}{};

		β await_ready()noexcept->bool{ return false; }
		β await_resume()noexcept->TResult=0;
		β await_suspend( coroutine_handle<typename TTask::promise_type> / *h* / )noexcept->void{ OriginalThreadParamPtr = { Threading::GetThreadDescription(), Threading::GetAppThreadHandle() }; }
		α AwaitResume()noexcept->void
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
		Await()noexcept=default;
		Await( string name )noexcept:_name{move(name)}{};

		β await_ready()noexcept->bool{ return false; }
		β await_resume()noexcept->AwaitResult=0;
		β await_suspend( HCoroutine )noexcept->void=0;//->void{ OriginalThreadParamPtr = { Threading::GetThreadDescription(), Threading::GetAppThreadHandle() }; }
		α AwaitSuspend()noexcept{ OriginalThreadParamPtr = { Threading::GetThreadDescription(), Threading::GetAppThreadHandle() }; }
		α AwaitResume()noexcept->void
		{
			if( _name.size() )
				DBG("({}){}::await_resume"sv, std::this_thread::get_id(), _name);
			if( OriginalThreadParamPtr )
				Threading::SetThreadInfo( *OriginalThreadParamPtr );
		}
	protected:
		optional<Threading::ThreadParam> OriginalThreadParamPtr;
		const string _name;
	};
#define Base Await
	struct IAwait : public Base
	{
		IAwait( SRCE )noexcept:_sl{sl}{};
		IAwait( string name, SRCE )noexcept:Base{move(name)},_sl{sl}{};
		virtual ~IAwait()=0;
		α await_suspend( HCoroutine h )noexcept->void override
		{
			Base::AwaitSuspend();
			_pPromise = &h.promise();
			if( auto& ro = _pPromise->get_return_object(); ro.HasResult() )
				ro.Clear();
		}
		α await_resume()noexcept->AwaitResult override{ AwaitResume(); return move(_pPromise->get_return_object().Result()); }
		//α Set( AwaitResult::Value&& x )->void{ ASSERT( _pPromise ); _pPromise->get_return_object().SetResult( move(x) ); }
		ⓣ Set( up<T>&& p )->void{ ASSERT( _pPromise ); _pPromise->get_return_object().SetResult<T>( move(p) ); }
		//α Get()noexcept(false)->sp<void>{ ASSERT( _pPromise ); return _pPromise->get_return_object().Get( _sl ); }

		source_location _sl;
	protected:
		Task::promise_type* _pPromise{ nullptr };
	};
	inline IAwait::~IAwait(){}

	//run synchronous function in coroutine pool thread.
	Τ struct PoolAwait final : IAwait//todo rename FromSyncAwait?
	{
		using base=IAwait;
		PoolAwait( function<up<T>()> fnctn )noexcept:_fnctn{fnctn}{};
		α await_suspend( HCoroutine h )noexcept->void override{ base::await_suspend(h); CoroutinePool::Resume( move(h) ); }

		AwaitResult await_resume()noexcept override;
	private:
		function<up<T>()> _fnctn;
	};
	//assynchronous function continues at end
	class FunctionAwait /*final*/ : public IAwait
	{
		using base=IAwait;
	public:
		FunctionAwait( function<Task(HCoroutine)> fnctn, SRCE, str name={} )noexcept:base{name, sl}, _fnctn2{fnctn}{};

		α await_suspend( HCoroutine h )noexcept->void override{ base::await_suspend( h ); _fnctn2( move(h) ); }
	private:
		function<Task(HCoroutine)> _fnctn2;
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
		ASSERT( !r.HasShared() );
		if( r.HasValue() )
			p.set_value( r.UP<T>() );
		else
			p.set_exception( r.Error()->Ptr() );
	}

	ⓣ Future( IAwait&& a )->std::future<up<T>>
	{
		auto p = std::promise<up<T>>();
		std::future<up<T>> f = p.get_future();
		CallAwait( move(p), move(a) );
		return f;
	}

	Ξ CallVAwait( std::promise<void>&& p_, IAwait&& a )->Task
	{
		auto p = move( p_ );
		AwaitResult r = co_await a;
		if( r.HasError() )
			p.set_exception( r.Error()->Ptr() );
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
		if( r.HasShared() )
			p.set_value( r.SP<T>(a._sl) );
		else
			p.set_exception( r.Error()->Ptr() );
	}

	ⓣ SFuture( IAwait&& a )->std::future<sp<T>>
	{
		std::promise<sp<T>> p;
		std::future<sp<T>> f = p.get_future();
		CallAwait( move(p), move(a) );
		return f;
	}

	struct AWrapper final : IAwait
	{
		using base=IAwait;
		AWrapper( function<Task(HCoroutine h)> fnctn, str name={} )noexcept:base{name}, _fnctn{fnctn}{};

		α await_suspend( HCoroutine h )noexcept->void override
		{
			base::await_suspend( h );
			_fnctn( move(h) );
		}
	private:
		function<Task(HCoroutine h)> _fnctn;
	};

	//template<class T>
	struct CancelAwait /*abstract*/ : Await
	{
		CancelAwait( ClientHandle& handle )noexcept:_hClient{ NextHandle() }{ handle = _hClient; }
		virtual ~CancelAwait()=0;
	protected:
		const ClientHandle _hClient;
	};
	/*template<class T>*/ inline CancelAwait::~CancelAwait() {}

	ⓣ PoolAwait<T>::await_resume()noexcept->AwaitResult
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