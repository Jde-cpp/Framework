﻿#include "Alarm.h"

#define var const auto

namespace Jde::Threading
{
	static sp<Jde::LogTag> _logTag{ Logging::Tag("alarm") };
	namespace detail{ α AlarmLogTag()ι->sp<Jde::LogTag>{return _logTag;} }
	Alarm _instance;
	flat_multimap<TimePoint,tuple<Handle,HCoroutine,std::source_location>> _coroutines; mutex _coroutineMutex;
	std::condition_variable _cv; mutex _mtx;

	α AlarmAwait::await_suspend( HCoroutine h )ι->void{
		Alarm::Add( _alarm, h, _hClient, _sl );
	}

	α Alarm::Add( TimePoint t, HCoroutine h, Coroutine::Handle myHandle, SL sl )ι->void{
		{
			lg _{_coroutineMutex};
			_coroutines.emplace( t, make_tuple(myHandle, h, sl) );
		}
		_instance.WakeUp();
		TRACESL( "({})Alarm::Add({})", myHandle, Chrono::ToString(Clock::now()-t) );
		if( t-Clock::now()<WakeDuration ){
			lg lk( _mtx );
			_cv.notify_one();
		}
	}

	α Alarm::Cancel( Coroutine::Handle h )ι->void{
		lg _{ _coroutineMutex };
		if( auto p = find_if(_coroutines, [h](var x){ return get<0>(x.second)==h;}); p!=_coroutines.end() ){
			SL sl{ get<2>(p->second) };
			TRACESL( "({})Alarm::Cancel()", get<0>(p->second) );
			get<1>( p->second ).destroy();
			_coroutines.erase( p );
		}
		else
			WARN( "Could not find handle {}."sv, h );
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
				TRACE( "Alarm wait until:  {}, calls={}"sv, LocalTimeDisplay(until, true, true), Calls() );
			/*var status =*/ _cv.wait_for( lk, until-now );
		}
		lg _{ _coroutineMutex };
		bool processed = false;
		for( auto p=_coroutines.begin(); p!=_coroutines.end() && p->first<Clock::now(); p = _coroutines.erase(p) ){
			processed = true;
			SL sl{ get<2>(p->second) };
			TRACESL( "({})Alarm::Resume()", get<0>(p->second) );
			Coroutine::CoroutinePool::Resume( move(get<1>(p->second)) );
		}
		return _coroutines.empty() ? std::nullopt : optional<bool>{ processed };
	}
	α Alarm::Shutdown()ι->void{
		DBG("Alarm::Shutdown()");
		_pThread->request_stop();
		lg _( _mtx );
		_cv.notify_one();
		IWorker::Shutdown();
		lg _2{ _coroutineMutex };
		for( auto p = _coroutines.begin(); p!=_coroutines.end(); p=_coroutines.erase(p) ){
			SL sl{ get<2>(p->second) };
			TRACESL( "({})Alarm::Removing Coroutine", get<0>(p->second) );
			_coroutines.clear();
		}
		DBG("~Alarm::Shutdown()");
	}
}