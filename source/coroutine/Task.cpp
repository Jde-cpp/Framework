#include <jde/coroutine/Task.h>

namespace Jde::Coroutine
{
	std::atomic<ClientHandle> _handleIndex{0};
	α NextHandle()noexcept->Handle{return ++_handleIndex;}

	std::atomic<ClientHandle> TaskHandle{0};
	α NextTaskHandle()noexcept->Handle{ return ++TaskHandle; }

	std::atomic<ClientHandle> TaskPromiseHandle{0};
	α NextTaskPromiseHandle()noexcept->Handle{ return ++TaskPromiseHandle; }

	α TaskResult::CheckUninitialized()noexcept->void
	{
		if( !Uninitialized() )
		{
			auto p = get<sp<void>>( _result );
			CRITICAL( "index={} p={}", _result.index(), p );
		}
	}

	α Task2::promise_type::unhandled_exception()noexcept->void
	{
		try
		{
			auto p = std::current_exception();
			if( p )
				std::rethrow_exception( p );
			else
				ERR( "unhandled_exception - no exception"sv );
		}
		catch( const IException& e )
		{
			e.Log();
			CRITICAL( "unhandled - {}", e.what() );
		}
		catch( const std::exception& e )
		{
			CRITICAL( "unhandled_exception ->{}"sv, e.what() );
		}
		catch( ... )
		{
			CRITICAL( "unhandled_exception"sv );
		}
	}
}
