#include <jde/framework/coroutine/Timer.h>
#include <boost/asio.hpp>
#include <jde/framework/thread/execution.h>

namespace Jde{
	α DurationTimer::Suspend()ι->void{
		auto io = Executor();
	  boost::asio::steady_timer t{ *io };
		t.expires_at( steady_clock::now()+_duration );
	  t.async_wait([this](const boost::system::error_code& ec){
		  if( !ec )
				Resume();
			else if( ec != boost::asio::error::operation_aborted )
				ResumeExp( BoostCodeException{ec} );
	  });
		Execution::Run();
	}
}
