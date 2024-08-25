#include "Alarm.h"

#define var const auto

namespace Jde::Threading{
	constexpr ELogTags _tags{ ELogTags::Scheduler };
	Alarm _instance;
	flat_multimap<TimePoint,tuple<Handle,HCoroutine,std::source_location>> _coroutines; mutex _coroutineMutex;
	std::condition_variable _cv; mutex _mtx;

	α AlarmAwait::Suspend()ι->void{
		Alarm::Add( _alarm, _h, _hClient, _sl );
	}

	α Alarm::Add( TimePoint t, HCoroutine h, Coroutine::Handle myHandle, SL sl )ι->void{
		{
			lg _{_coroutineMutex};
			_coroutines.emplace( t, make_tuple(myHandle, h, sl) );
		}
		_instance.WakeUp();
		Trace( sl, _tags, "({})Alarm::Add({})", myHandle, Chrono::ToString(Clock::now()-t) );
		if( t-Clock::now()<WakeDuration ){
			lg lk( _mtx );
			_cv.notify_one();
		}
	}

	α Alarm::Cancel( Coroutine::Handle h )ι->void{
		lg _{ _coroutineMutex };
		if( auto p = find_if(_coroutines, [h](var x){ return get<0>(x.second)==h;}); p!=_coroutines.end() ){
			SL sl{ get<2>(p->second) };
			Trace( sl, _tags, "({})Alarm::Cancel()", get<0>(p->second) );
			get<1>( p->second ).destroy();
			_coroutines.erase( p );
		}
		else
			Warning( _tags, "Could not find handle {}.", h );
	}

	α Next()ι->optional<TimePoint>{
		lg _{ _coroutineMutex };//only shared
		return _coroutines.size() ? optional<TimePoint>{ _coroutines.begin()->first }: nullopt;
	}
	α Alarm::Poll()ι->optional<bool>{
		static uint i=0;
		{
			std::unique_lock<std::mutex> lk( _mtx );
			var now = Clock::now();
			var dflt = now+WakeDuration;
			var next = Next().value_or(dflt);
			var until = std::min( dflt, next );
			if( ++i%10==0 )
				Trace( _tags, "Alarm wait until:  {}, calls={}"sv, LocalTimeDisplay(until, true, true), Calls() );
			/*var status =*/ _cv.wait_for( lk, until-now );
		}
		lg _{ _coroutineMutex };
		bool processed = false;
		for( auto p=_coroutines.begin(); p!=_coroutines.end() && p->first<Clock::now(); p = _coroutines.erase(p) ){
			processed = true;
			SL sl{ get<2>(p->second) };
			Trace( sl, _tags, "({})Alarm::Resume()", get<0>(p->second) );
			Coroutine::CoroutinePool::Resume( move(get<1>(p->second)) );
		}
		return _coroutines.empty() ? std::nullopt : optional<bool>{ processed };
	}
	α Alarm::Shutdown( bool terminate )ι->void{
		Debug( _tags, "Alarm::Shutdown()" );
		_pThread->request_stop();
		lg _( _mtx );
		_cv.notify_one();
		IWorker::Shutdown( terminate );
		lg _2{ _coroutineMutex };
		for( auto p = _coroutines.begin(); p!=_coroutines.end(); p=_coroutines.erase(p) ){
			SL sl{ get<2>(p->second) };
			Trace( sl, _tags, "({})Alarm::Removing Coroutine", get<0>(p->second) );
			_coroutines.clear();
		}
		Debug( _tags, "~Alarm::Shutdown()" );
	}
}