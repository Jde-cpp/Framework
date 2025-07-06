#include <jde/framework/coroutine/Timer.h>
#include <jde/framework/thread/execution.h>

namespace Jde{
	DurationTimer::DurationTimer( steady_clock::duration duration, SL sl )ι:
		VoidAwait<>{sl},
		_duration{duration},
		_ctx{ Executor() },
		_timer{ *_ctx }
	{}
	α DurationTimer::Restart()ι->void{
		_timer.expires_after( _duration );
	  _timer.async_wait([this](const boost::system::error_code& ec){
			ASSERT( _h );
		  if( !ec )
				Resume();
			else if( ec != boost::asio::error::operation_aborted )
				ResumeExp( BoostCodeException{ec} );
	  });
	}
	α DurationTimer::Suspend()ι->void{
		Restart();
		Execution::Run();
	}
}
