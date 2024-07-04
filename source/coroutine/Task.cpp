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

	α Task::promise_type::unhandled_exception()ι->void{
		try{
			throw;
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
