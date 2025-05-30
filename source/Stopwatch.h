#pragma once
#ifndef STOPWATCH_H
#define STOPWATCH_H
#include <chrono>
#include <map>
#include <memory>
#include <string>

namespace Jde{
	enum class StopwatchTypes{
		Other,
		Calculate,
		ComputeCost,
		Convert,
		Copy,
		Create,
		GradientDescent,
		Prefix,
		ReadFile,
		WriteFile,
		ServerCall
	};
	struct Γ Stopwatch final{
		using SDuration=steady_clock::duration;
		using STimePoint=steady_clock::time_point;
		Stopwatch( sv what, bool started=true, SRCE )ι;
		Stopwatch( sv what, ELogTags tags, bool started=true, SRCE )ι;
		Stopwatch( Stopwatch* pParent, sv what="", sv instance="", bool started=true, SRCE )ι;
		~Stopwatch();

		α Finish( sv description )ι->void;
		α Finish( bool remove=true )ι->void;
		α Progress( uint count, uint total=0 )Ι->string{ return Progress( count, total, "" ); }
		α Progress( uint count, uint total, sv context, bool force=false )Ι->string;
		SDuration Elapsed()Ι;
		Ω FormatSeconds( const SDuration& seconds )ι->string;
		Ω FormatCount( double total )ι->string;
		α Pause()ι->void;
		α UnPause()ι->void;
		α Restart()ι->void{ _start = steady_clock::now(); }
		α StartTime()Ι->STimePoint{ return _start; }
	private:
		α Output( sv what, const SDuration& elapsed, bool logMemory, SRCE )ι->void;

		const string _what;
		const string _instance;
		steady_clock::time_point _start;
		ELogTags _tags;
		Stopwatch* _pParent{};
		flat_map<string,SDuration> _children;

		bool _finished{};
		const bool _logMemory{};
		static SDuration _minimumToLog;
		mutable SDuration _previousProgressElapsed{0};
		static flat_map<string,SDuration> _accumulations;

		STimePoint _startPause;
		SDuration _elapsedPause{};
		source_location _sl;
	};
}
#endif