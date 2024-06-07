#include <jde/coroutine/Task.h>
#include "../threading/Thread.h"
#include <nlohmann/json.hpp>

namespace Jde::Coroutine{
	static sp<LogTag> _logTag{ Logging::Tag("coroutine") };

	std::atomic<ClientHandle> _handleIndex{0};
	α NextHandle()ι->Handle{return ++_handleIndex;}

	std::atomic<ClientHandle> TaskHandle{0};
	α NextTaskHandle()ι->Handle{ return ++TaskHandle; }

	std::atomic<ClientHandle> TaskPromiseHandle{0};
	α NextTaskPromiseHandle()ι->Handle{ return ++TaskPromiseHandle; }

	AwaitResult::~AwaitResult(){
	}
	α AwaitResult::CheckUninitialized()ι->void{
		if( !Uninitialized() )
			CRITICAL( "Uninitialized - index={}", _result.index() );
	}
	atomic<uint> TaskIndex{ 0 };
/*	Task::Task()ι:i{++TaskIndex}
	{
		TRACE( "({:x}-{}) created", (uint)this, i );
		if( i==7 )
			BREAK;
	}
	Task::Task( Task&& x )ι:
		_result{ move(x._result) }
	{
		i = ++TaskIndex;
		TRACE( "({:x}-{}) moved ({:x})-{}", (uint)this, i, (uint)&x, x.i );
	}
	Task::Task( const Task& x )ι:
		i{++TaskIndex}
	{
		TRACE( "({:x}-{}) copied ({:x})-{}", (uint)this, i, (uint)&x, x.i );
	}

	Task::~Task()ι
	{
		TRACE( "({:x}-{}) removed", (uint)this, i );
	}
	*/
	α Task::promise_type::unhandled_exception()ι->void{
		try{
			BREAK;
			if( auto p = std::current_exception(); p )
				std::rethrow_exception( p );
			else
				CRITICAL( "unhandled_exception - no exception"sv );
		}
		catch( CoException& e ){
			e.Resume( *this );
		}
		catch( IException& e ){
			//e.Log();
			// if( _unhandledResume )
			// {
			// 	_pReturnObject->SetResult( move(e) );
			// 	_unhandledResume.resume();
			// }
			// else
				CRITICAL( "Jde::Task::promise_type::unhandled_exception - {}", e.what() );
		}
		catch( const nlohmann::json::exception& e ){
			CRITICAL( "Jde::Task::promise_type::unhandled_exception - json exception - {}"sv, e.what() );
		}
		catch( const std::exception& e ){
			CRITICAL( "Jde::Task::promise_type::unhandled_exception ->{}"sv, e.what() );
		}
		catch( ... ){
			CRITICAL( "Jde::Task::promise_type::unhandled_exception"sv );
		}
	}
}
