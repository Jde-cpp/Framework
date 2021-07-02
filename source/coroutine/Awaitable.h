#pragma once
#include "Coroutine.h"
#include <jde/coroutine/Task.h>
namespace Jde::Coroutine
{
	using ClientHandle = uint;

	template<typename TTask>
	struct IAwaitable
	{
		//typedef typename TTask::promise_type TPromise;
		using TResult=typename TTask::TResult;
		using TPromise=typename TTask::promise_type;
		using THandle=coroutine_handle<TPromise>;
		IAwaitable()noexcept=default;
		IAwaitable( str name )noexcept:_name{name}{};
		virtual ~IAwaitable()=0;

		virtual α await_ready()noexcept->bool{ return false; }
		virtual TResult await_resume()noexcept=0;//{ LogAwaitResume(); }//returns the result value for co_await expression.
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
	template<typename TTask> IAwaitable<TTask>::~IAwaitable(){}

	//template<class T>
	struct NotReadyErrorAwaitable /*final*/ : IAwaitable<Task2>
	{
		using base=IAwaitable<Task2>;
		NotReadyErrorAwaitable( str name={} )noexcept:base{name}{};
		bool await_ready()noexcept override{ return false; }
		void await_suspend( typename base::THandle h )noexcept override{ base::await_suspend( h ); _pPromise = &h.promise(); }
		typename base::TResult await_resume()noexcept override{ return _pPromise->get_return_object().Result; }
	protected:
		typename base::TPromise* _pPromise{ nullptr };
	};

//	template<class T>
	struct ErrorAwaitable final : NotReadyErrorAwaitable//<T>
	{
		using base=NotReadyErrorAwaitable;
		ErrorAwaitable( function<sp<void>()> fnctn )noexcept:_fnctn{fnctn}{};
		void await_suspend( typename base::THandle h )noexcept override;
	private:
		function<sp<void>()> _fnctn;
	};

	//template<class T, class TReturn=void>
	struct ErrorAwaitableAsync final : NotReadyErrorAwaitable
	{
		using base=NotReadyErrorAwaitable;
		ErrorAwaitableAsync( function<Task2(typename base::THandle)> fnctn, str name={} )noexcept:base{name}, _fnctn2{fnctn}{};

		void await_suspend( typename base::THandle h )noexcept override{ base::await_suspend( h ); _fnctn2( move(h) ); }
	private:
		function<Task2(typename base::THandle)> _fnctn2;
	};

//Coroutine::TaskError<Tick>
	template<class TTask>
	struct CancelAwaitable : IAwaitable<TTask>
	{
		CancelAwaitable( ClientHandle& handle )noexcept:_hClient{ NextHandle() }{ handle = _hClient; }
	protected:
		const ClientHandle _hClient;
	};

	//template<class T>
	inline void ErrorAwaitable::await_suspend( typename base::THandle h )noexcept
	{
		base::await_suspend( h );
		std::thread( [this,h2=move(h)]()mutable//TODO move to thread pool
		{
			try
			{
				sp<void> x = _fnctn();
				h2.promise().get_return_object().Result = x;
				DBG( "Awaitable - Calling resume()."sv );
				Coroutine::CoroutinePool::Resume( move(h2) );//TODO after moving to thread pool decide if this will get run by the current pool or Coroutine::CoroutinePool.
			}
			catch( const std::exception& e )
			{
				auto p = std::make_exception_ptr( e );
				h2.promise().get_return_object().Result = p;
				DBG( "Awaitable - Calling resume() with error."sv );
				Coroutine::CoroutinePool::Resume( move(h2) );
			}
		}).detach();
	}

}