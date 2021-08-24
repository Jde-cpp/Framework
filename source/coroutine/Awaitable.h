#pragma once
#include <jde/Exports.h>
#include <jde/coroutine/Task.h>
#include "Coroutine.h"

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

		virtual α await_ready()noexcept->bool{ return false; }
		virtual TResult await_resume()noexcept=0;
		inline virtual void await_suspend( coroutine_handle<Task2::promise_type> /*h*/ )noexcept{ OriginalThreadParamPtr = { Threading::GetThreadDescription(), Threading::GetAppThreadHandle() }; }
		inline void AwaitResume()noexcept
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

	struct IAwaitable /*abstract*/ : TAwaitable<Task2>
	{
		using base=TAwaitable<Task2>;
		virtual ~IAwaitable()=0;

		IAwaitable( str name={} )noexcept:base{name}{};
		void await_suspend( typename base::THandle h )noexcept override{ base::await_suspend( h ); _pPromise = &h.promise(); }
		typename base::TResult await_resume()noexcept override{ AwaitResume(); return _pPromise->get_return_object().GetResult(); }
	protected:
		typename base::TPromise* _pPromise{ nullptr };
	};
	inline IAwaitable::~IAwaitable(){}

	struct AsyncAwaitable final : IAwaitable
	{
		AsyncAwaitable( function<sp<void>()> fnctn )noexcept:_fnctn{fnctn}{};
		void await_suspend( IAwaitable::THandle h )noexcept override{ base::await_suspend(h); CoroutinePool::Resume( move(h) ); }

		TaskResult await_resume()noexcept override;
	private:
		function<sp<void>()> _fnctn;
	};

	struct FunctionAwaitable final : IAwaitable
	{
		using base=IAwaitable;
		FunctionAwaitable( function<Task2(typename base::THandle)> fnctn, str name={} )noexcept:base{name}, _fnctn2{fnctn}{};

		void await_suspend( typename base::THandle h )noexcept override{ base::await_suspend( h ); _fnctn2( move(h) ); }
	private:
		function<Task2(HCoroutine)> _fnctn2;
	};
	template<class T>
	struct Promise : std::promise<T>
	{
		~Promise(){ DBG("~Promise"sv); }
	};
	ⓣ CallAwaitable( up<Promise<sp<T>>> pPromise, IAwaitable&& a )->Task2
	{
		TaskResult r = co_await a;
		if( r.HasValue() )
			pPromise->set_value( r.Get<T>() );
		else
			pPromise->set_exception( r.Error() );
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

		void await_suspend( HCoroutine h )noexcept override{ base::await_suspend( h ); _fnctn( move(h) ); }
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

	inline TaskResult AsyncAwaitable::await_resume()noexcept
	{
		AwaitResume();
		try
		{
			return TaskResult{ _fnctn() };
		}
		catch( const std::exception& e )
		{
			return TaskResult{ std::make_exception_ptr(e) };
		}
	}
}