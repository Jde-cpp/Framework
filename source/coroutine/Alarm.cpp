#include "Alarm.h"

#define var const auto

namespace Jde::Threading
{
	Alarm _instance;
	void AlarmAwaitable::await_suspend( AlarmAwaitable::THandle h )noexcept
	{
		Alarm::Add( _alarm, h, _hClient );
	}

	void Alarm::Add( TimePoint t, AlarmAwaitable::THandle h, Coroutine::Handle myHandle )noexcept
	{
		_instance.Add2( t, h, myHandle );
	}
	void Alarm::Add2( TimePoint t, AlarmAwaitable::THandle h, Coroutine::Handle myHandle )noexcept
	{
		{
			unique_lock l{_coroutineMutex};
			_coroutines.emplace( t, make_tuple(myHandle, h) );
			DBG( "Alarm::Add2({})"sv, Chrono::ToString(Clock::now()-t) );
		}
		if( t-Clock::now()<WakeDuration )
		{
			std::unique_lock<std::mutex> lk( _mtx );
			_cv.notify_one();
		}
	}

	void Alarm::Cancel( Coroutine::Handle h )noexcept
	{
		_instance.Cancel2( h );
	}

	void Alarm::Cancel2( Coroutine::Handle h )noexcept
	{
		unique_lock l{ _coroutineMutex };
		if( auto p = std::find_if(_coroutines.begin(), _coroutines.end(), [h](var x){ return get<0>(x.second)==h;}); p!=_coroutines.end() )
		{
			DBG( "Cancel({})"sv, h );
			get<1>( p->second ).destroy();
			_coroutines.erase( p );
		}
		else
			DBG( "Could not find handle {}."sv, h );
	}

	optional<TimePoint> Alarm::Next()noexcept
	{
		unique_lock l{ _coroutineMutex };//only shared
		return _coroutines.size() ? _coroutines.begin()->first : optional<TimePoint>{};
	}
	optional<bool> Alarm::Poll()noexcept
	{
		static uint i=0;
		{
			std::unique_lock<std::mutex> lk( _mtx );
			var now = Clock::now();
			var dflt = now+WakeDuration;
			var next = Next().value_or(dflt);
			var until = std::min( dflt, next );
			if( ++i%10==0 )
				DBG( "Alarm wait until:  {}"sv, ToIsoString(until) );
			/*var status =*/ _cv.wait_for( lk, until-now );
		}
		unique_lock l{ _coroutineMutex };
		bool processed = false;
		for( auto p=_coroutines.begin(); p!=_coroutines.end() && p->first<Clock::now(); p = _coroutines.erase(p) )
		{
			processed = true;
			Coroutine::CoroutinePool::Resume( move(get<1>(p->second)) );
		}
		return _coroutines.empty() ? std::nullopt : optional<bool>{ processed };
	}
	void Alarm::Shutdown()noexcept
	{
		_pThread->request_stop();
		std::unique_lock<std::mutex> lk( _mtx );
		_cv.notify_one();
		IWorker::Shutdown();
	}
}
