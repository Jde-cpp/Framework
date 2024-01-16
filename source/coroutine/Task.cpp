#include <jde/coroutine/Task.h>
#include <nlohmann/json.hpp>

namespace Jde::Coroutine
{
	std::atomic<ClientHandle> _handleIndex{0};
	α NextHandle()noexcept->Handle{return ++_handleIndex;}

	std::atomic<ClientHandle> TaskHandle{0};
	α NextTaskHandle()noexcept->Handle{ return ++TaskHandle; }

	std::atomic<ClientHandle> TaskPromiseHandle{0};
	α NextTaskPromiseHandle()noexcept->Handle{ return ++TaskPromiseHandle; }

	α AwaitResult::CheckUninitialized()noexcept->void
	{
		if( !Uninitialized() )
		{
			//auto p = get<sp<void>>( _result );
			CRITICAL( "index={}", _result.index() );
		}
	}
	atomic<uint> TaskIndex{ 0 };
/*	Task::Task()noexcept:i{++TaskIndex}
	{
		DBG( "({:x}-{}) created", (uint)this, i );
		if( i==7 )
			BREAK;
	}
	Task::Task( Task&& x )noexcept:
		_result{ move(x._result) }
	{
		i = ++TaskIndex;
		DBG( "({:x}-{}) moved ({:x})-{}", (uint)this, i, (uint)&x, x.i );
	}
	Task::Task( const Task& x )noexcept:
		i{++TaskIndex}
	{
		DBG( "({:x}-{}) copied ({:x})-{}", (uint)this, i, (uint)&x, x.i );
	}

	Task::~Task()noexcept
	{
		DBG( "({:x}-{}) removed", (uint)this, i );
	}
	*/
	α Task::promise_type::unhandled_exception()noexcept->void
	{
		try{
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
				CRITICAL( "unhandled - {}", e.what() );
		}
		catch( const nlohmann::json::exception& e ){
			CRITICAL( "json exception - {}"sv, e.what() );
		}
		catch( const std::exception& e ){
			CRITICAL( "unhandled_exception ->{}"sv, e.what() );
		}
		catch( ... ){
			CRITICAL( "unhandled_exception"sv );
		}
	}
}
