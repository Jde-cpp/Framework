#pragma once
#include "Coroutine.h"
#include <jde/coroutine/Task.h>
namespace Jde::Coroutine
{
	using ClientHandle = uint;

	template<class TTask=Task2>
	struct TAwaitable
	{
		using TResult=typename TTask::TResult;
		using TPromise=typename TTask::promise_type;
		using THandle=coroutine_handle<TPromise>;
		TAwaitable()noexcept=default;
		TAwaitable( str name )noexcept:_name{name}{};
		virtual ~TAwaitable()=0;

		virtual α await_ready()noexcept->bool{ return false; }
		virtual TResult await_resume()noexcept=0;
		virtual α await_suspend( THandle /*h*/ )noexcept->void{ OriginalThreadParamPtr = { Threading::GetThreadDescription(), Threading::GetAppThreadHandle() }; }
		void AwaitResume()noexcept
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
	template<class T> TAwaitable<T>::~TAwaitable(){}

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
		using base=IAwaitable;
		AsyncAwaitable( function<sp<void>()> fnctn )noexcept:_fnctn{fnctn}{};
		void await_suspend( typename base::THandle h )noexcept override;
	private:
		function<sp<void>()> _fnctn;
	};

	struct FunctionAwaitable final : IAwaitable //TODO try FunctionType like InteruptableThread
	{
		using base=IAwaitable;
		FunctionAwaitable( function<Task2(typename base::THandle)> fnctn, str name={} )noexcept:base{name}, _fnctn2{fnctn}{};

		void await_suspend( typename base::THandle h )noexcept override{ base::await_suspend( h ); _fnctn2( move(h) ); }
	private:
		function<Task2(typename base::THandle)> _fnctn2;
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

	inline void AsyncAwaitable::await_suspend( typename base::THandle h )noexcept
	{
		base::await_suspend( h );
		std::thread( [this,h2=move(h)]()mutable//TODO move to thread pool
		{
			try
			{
				sp<void> x = _fnctn();
				h2.promise().get_return_object().SetResult( x );
				DBG( "Awaitable - Calling resume()."sv );
				Coroutine::CoroutinePool::Resume( move(h2) );//TODO after moving to thread pool decide if this will get run by the current pool or Coroutine::CoroutinePool.
			}
			catch( const std::exception& e )
			{
				auto p = std::make_exception_ptr( e );
				h2.promise().get_return_object().SetResult( p );
				DBG( "Awaitable - Calling resume() with error."sv );
				Coroutine::CoroutinePool::Resume( move(h2) );
			}
		}).detach();
	}
}