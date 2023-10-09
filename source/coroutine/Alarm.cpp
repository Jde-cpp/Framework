#include "Alarm.h"

#define var const auto

namespace Jde::Threading
{
	const LogTag& Alarm::_logLevel{ Logging::TagLevel("alarm") };
	Alarm _instance;
	flat_multimap<TimePoint,tuple<Handle,HCoroutine>> _coroutines; mutex _coroutineMutex;
	std::condition_variable _cv; mutex _mtx;

	α AlarmAwait::await_suspend( HCoroutine h )ι->void
	{
		Alarm::Add( _alarm, h, _hClient );
	}

	α Alarm::Add( TimePoint t, HCoroutine h, Coroutine::Handle myHandle )ι->void
	{
		{
			unique_lock l{_coroutineMutex};
			_coroutines.emplace( t, make_tuple(myHandle, h) );
		}
		_instance.WakeUp();
		LOG( "({})Alarm::Add({})"sv, myHandle, Chrono::ToString(Clock::now()-t) );
		if( t-Clock::now()<WakeDuration )
		{
			std::unique_lock<std::mutex> lk( _mtx );
			_cv.notify_one();
		}
	}

	α Alarm::Cancel( Coroutine::Handle h )ι->void
	{
		unique_lock l{ _coroutineMutex };
		if( auto p = std::find_if(_coroutines.begin(), _coroutines.end(), [h](var x){ return get<0>(x.second)==h;}); p!=_coroutines.end() )
		{
			LOG( "({})Alarm::Cancel()"sv, get<0>(p->second) );
			get<1>( p->second ).destroy();
			_coroutines.erase( p );
		}
		else
			LOG( "Could not find handle {}."sv, h );
	}

	α Next()ι->optional<TimePoint>
	{
		unique_lock l{ _coroutineMutex };//only shared
		return _coroutines.size() ? optional<TimePoint>{ _coroutines.begin()->first }: nullopt;
	}
	α Alarm::Poll()ι->optional<bool>
	{
		static uint i=0;
		{
			std::unique_lock<std::mutex> lk( _mtx );
			var now = Clock::now();
			var dflt = now+WakeDuration;
			var next = Next().value_or(dflt);
			var until = std::min( dflt, next );
			if( ++i%10==0 )
				LOG( "Alarm wait until:  {}, calls={}"sv, LocalTimeDisplay(until, true, true), Calls() );
			/*var status =*/ _cv.wait_for( lk, until-now );
		}
		unique_lock l{ _coroutineMutex };
		bool processed = false;
		for( auto p=_coroutines.begin(); p!=_coroutines.end() && p->first<Clock::now(); p = _coroutines.erase(p) )
		{
			processed = true;
			TRACE( "({})Alarm::Resume()"sv, get<0>(p->second) );
			Coroutine::CoroutinePool::Resume( move(get<1>(p->second)) );
		}
		return _coroutines.empty() ? std::nullopt : optional<bool>{ processed };
	}
	α Alarm::Shutdown()ι->void
	{
		_pThread->request_stop();
		std::unique_lock<std::mutex> lk( _mtx );
		_cv.notify_one();
		IWorker::Shutdown();
	}
}