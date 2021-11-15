#pragma once
#include <jde/Exports.h>
#include <jde/coroutine/Task.h>
#include "Coroutine.h"
namespace Jde{ using namespace Jde::Coroutine; }
namespace Jde::Coroutine
{
	using ClientHandle = uint;
	using HCoroutine = coroutine_handle<Task2::promise_type>;
	template<class TTask=Task2>
	struct TAwaitable
	{
		using TResult=typename TTask::TResult;
		using TPromise=typename TTask::promise_type;
		using THandle=coroutine_handle<TPromise>;
		TAwaitable()noexcept=default;
		TAwaitable( str name )noexcept:_name{name}{};

		β await_ready()noexcept->bool{ return false; }
		β await_resume()noexcept->TResult=0;
		β await_suspend( coroutine_handle<typename TTask::promise_type> /*h*/ )noexcept->void{ OriginalThreadParamPtr = { Threading::GetThreadDescription(), Threading::GetAppThreadHandle() }; }
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

	class IAwaitable : public TAwaitable<Task2>
	{
		using base=TAwaitable<Task2>;
	public:
		IAwaitable( str name={} )noexcept:base{name}{};
		virtual ~IAwaitable()=0;
		α await_suspend( HCoroutine h )noexcept->void override
		{
			base::await_suspend( h );
			_pPromise = &h.promise();
			if( auto& ro = _pPromise->get_return_object(); ro.HasResult() )
				ro.Clear();
		}
		α await_resume()noexcept->TaskResult override{ AwaitResume(); return _pPromise->get_return_object().GetResult(); }
	protected:
		Task2::promise_type* _pPromise{ nullptr };
	};
	inline IAwaitable::~IAwaitable(){}

	struct AsyncAwaitable final : IAwaitable
	{
		using base=IAwaitable;
		AsyncAwaitable( function<sp<void>()> fnctn )noexcept:_fnctn{fnctn}{};
		α await_suspend( IAwaitable::THandle h )noexcept->void override{ base::await_suspend(h); CoroutinePool::Resume( move(h) ); }

		TaskResult await_resume()noexcept override;
	private:
		function<sp<void>()> _fnctn;
	};

	class FunctionAwaitable /*final*/ : public IAwaitable
	{
		using base=IAwaitable;
	public:
		FunctionAwaitable( function<Task2(typename base::THandle)> fnctn, str name={} )noexcept:base{name}, _fnctn2{fnctn}{};

		α await_suspend( typename base::THandle h )noexcept->void override{ base::await_suspend( h ); _fnctn2( move(h) ); }
	private:
		function<Task2(HCoroutine)> _fnctn2;
	};
	template<class T>
	struct Promise : std::promise<T>
	{};
	ⓣ CallAwaitable( up<Promise<sp<T>>> pPromise, IAwaitable&& a )->Task2
	{
		TaskResult r = co_await a;
		if( r.HasValue() )
			pPromise->set_value( r.Get<T>() );
		else
			pPromise->set_exception( r.Error()->Ptr() );
	}
	ⓣ Future( IAwaitable&& a )->std::future<sp<T>>
	{
		auto p = make_unique<Promise<sp<T>>>();
		std::future<sp<T>> f = p->get_future();
		CallAwaitable( move(p), move(a) );
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

	template<class T>
	struct CancelAwaitable /*abstract*/ : TAwaitable<T>
	{
		CancelAwaitable( ClientHandle& handle )noexcept:_hClient{ NextHandle() }{ handle = _hClient; }
		virtual ~CancelAwaitable()=0;
	protected:
		const ClientHandle _hClient;
	};
	template<class T> CancelAwaitable<T>::~CancelAwaitable(){}

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