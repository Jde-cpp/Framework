#include <jde/framework/coroutine/TaskOld.h>
#include "../threading/Thread.h"

namespace Jde::Coroutine{
	constexpr ELogTags _tags{ ELogTags::Threads };

	std::atomic<ClientHandle> _handleIndex{0};
	α NextHandle()ι->Handle{return ++_handleIndex;}

	std::atomic<ClientHandle> TaskHandle{0};
	α NextTaskHandle()ι->Handle{ return ++TaskHandle; }

	std::atomic<ClientHandle> TaskPromiseHandle{0};
	α NextTaskPromiseHandle()ι->Handle{ return ++TaskPromiseHandle; }

	α AwaitResult::CheckUninitialized()ι->void{
		if( !Uninitialized() )
			Critical( _tags, "Uninitialized - index={}", _result.index() );
	}

	α Task::promise_type::unhandled_exception()ι->void{
		try{
			throw;
		}
		catch( CoException& e ){
			e.Resume( *this );
		}
		catch( IException& e ){
			Critical( _tags, "Jde::Task::promise_type::unhandled_exception - {}", e.what() );
		}
		catch( const std::exception& e ){
			Critical( _tags, "Jde::Task::promise_type::unhandled_exception ->{}", e.what() );
		}
		catch( ... ){
			Critical( _tags, "Jde::Task::promise_type::unhandled_exception" );
		}
	}
}
