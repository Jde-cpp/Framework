#pragma once
#include <jde/Exports.h>
#include <jde/coroutine/Task.h>
#include "Coroutine.h"
namespace Jde{ using namespace Jde::Coroutine; }
namespace Jde::Coroutine
{
	using ClientHandle = uint;
	using HCoroutine = coroutine_handle<Task2::promise_type>;

	/*https://stackoverflow.com/questions/44960760/msvc-dll-exporting-class-that-inherits-from-template-cause-lnk2005-already-defin
	template<class TTask=Task2>
	struct TAwaitable
	{
		using TResult=typename TTask::TResult;
		using TPromise=typename TTask::promise_type;
		using THandle=coroutine_handle<TPromise>;
		TAwaitable()noexcept=default;
		TAwaitable( string name )noexcept:_name{move(name)}{};

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
	template struct TAwaitable<Task2>;
	*/
	struct Await
	{
		Await()noexcept=default;
		Await( string name )noexcept:_name{move(name)}{};

		β await_ready()noexcept->bool{ return false; }
		β await_resume()noexcept->TaskResult=0;
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
//		uint ThreadHandle;
//		string ThreadName;
		const string _name;
	};
#define base Await
	struct IAwaitable : public base
	{
		IAwaitable( SRCE )noexcept:_sl{sl}{};
		IAwaitable( string name, SRCE )noexcept:base{move(name)},_sl{sl}{};
		virtual ~IAwaitable()=0;
		α await_suspend( HCoroutine h )noexcept->void override
		{
			base::AwaitSuspend();
			_pPromise = &h.promise();
			if( auto& ro = _pPromise->get_return_object(); ro.HasResult() )
				ro.Clear();
		}
		α await_resume()noexcept->TaskResult override{ AwaitResume(); return _pPromise->get_return_object().GetResult(); }
		α Set( std::variant<sp<void>,sp<TaskResult::TException>>&& x )->void{ ASSERT( _pPromise ); _pPromise->get_return_object().SetResult( move(x) ); }
		α Get()noexcept(false)->sp<void>{ ASSERT( _pPromise ); return _pPromise->get_return_object().Get( _sl ); }

		source_location _sl;
	protected:
		Task2::promise_type* _pPromise{ nullptr };
	};
#undef base
	inline IAwaitable::~IAwaitable(){}
	//run synchronous function in coroutine pool thread.
	struct AsyncAwaitable final : IAwaitable//todo rename FromSyncAwait?
	{
		using base=IAwaitable;
		AsyncAwaitable( function<sp<void>()> fnctn )noexcept:_fnctn{fnctn}{};
		α await_suspend( HCoroutine h )noexcept->void override{ base::await_suspend(h); CoroutinePool::Resume( move(h) ); }

		TaskResult await_resume()noexcept override;
	private:
		function<sp<void>()> _fnctn;
	};

	class FunctionAwaitable /*final*/ : public IAwaitable
	{
		using base=IAwaitable;
	public:
		FunctionAwaitable( function<Task2(HCoroutine)> fnctn, SRCE, str name={} )noexcept:base{name, sl}, _fnctn2{fnctn}{};

		α await_suspend( HCoroutine h )noexcept->void override{ base::await_suspend( h ); _fnctn2( move(h) ); }
	private:
		function<Task2(HCoroutine)> _fnctn2;
	};
	template<class T>
	struct Promise : std::promise<T>
	{
		~Promise(){}
	};
	ⓣ CallAwaitable( up<Promise<sp<T>>> pPromise, IAwaitable&& a )->Task2
	{
		TaskResult r = co_await a;
		if( r.HasValue() )
			pPromise->set_value( r.Get<T>() );
		else
			pPromise->set_exception( r.Error()->Ptr() );
		//var d = GetThreadDscrptn();
		//SetThreadDscrptn( d.substr(0, d.size()-9) );todo make own future class, override get.
	}
	ⓣ Future( IAwaitable&& a )->std::future<sp<T>>
	{
		auto p = make_unique<Promise<sp<T>>>();
		std::future<sp<T>> f = p->get_future();
		CallAwaitable( move(p), move(a) );
		//Threading::SetThreadDscrptn( format("{} - Future", Threading::GetThreadDescription()) );
		return f;
	}
	ⓣ Future( up<IAwaitable> a )->std::future<sp<T>>
	{
		auto p = make_unique<Promise<sp<T>>>();
		std::future<sp<T>> f = p->get_future();
		CallAwaitable( move(p), move(*a) );
		return f;
	}
	struct AWrapper final : IAwaitable
	{
		using base=IAwaitable;
		AWrapper( function<Task2(HCoroutine h)> fnctn, str name={} )noexcept:base{name}, _fnctn{fnctn}{};

		α await_suspend( HCoroutine h )noexcept->void override
		{
			base::await_suspend( h );
			_fnctn( move(h) );
		}
	private:
		function<Task2(HCoroutine h)> _fnctn;
	};

	//template<class T>
	struct CancelAwaitable /*abstract*/ : Await
	{
		CancelAwaitable( ClientHandle& handle )noexcept:_hClient{ NextHandle() }{ handle = _hClient; }
		virtual ~CancelAwaitable()=0;
	protected:
		const ClientHandle _hClient;
	};
	/*template<class T>*/ inline CancelAwaitable::~CancelAwaitable() {}

	Ξ AsyncAwaitable::await_resume()noexcept->TaskResult
	{
		AwaitResume();
		try
		{
			return TaskResult{ _fnctn() };
		}
		catch( IException& e )
		{
			return TaskResult{ e.Clone() };
		}
	}
}