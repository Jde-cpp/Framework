﻿#pragma once
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <jde/Exports.h>

using std::string;

namespace Jde
{
	enum class StopwatchTypes
	{
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
	struct Γ Stopwatch
	{
		using SClock=std::chrono::steady_clock;
		using SDuration=SClock::duration;
		using STimePoint=SClock::time_point;
		Stopwatch( sv what, bool started=true )noexcept;
		Stopwatch( Stopwatch* pParent, sv what="", sv instance="", bool started=true )noexcept;
		virtual ~Stopwatch();

		void Finish( sv description );
		void Finish( bool remove=true );
		string Progress( uint count, uint total=0 )const{ return Progress( count, total, "" ); }
		string Progress( uint count, uint total, sv context, bool force=false )const;
		SDuration Elapsed()const;
		static std::string FormatSeconds( const SDuration& seconds );
		static std::string FormatCount( double total );
		void Pause();
		void UnPause();
		void Restart(){ _start = std::chrono::steady_clock::now(); }
	private:
		static void Output( sv what, const SDuration& elapsed, bool logMemory, SRCE );

		const std::string _what;
		const std::string _instance;
		std::chrono::steady_clock::time_point _start;

		Stopwatch* _pParent{nullptr};
		std::map<std::string,SDuration> _children;

		bool _finished{false};
		const bool _logMemory{false};
		static SDuration _minimumToLog;
		mutable SDuration _previousProgressElapsed{0};
		static flat_map<string,SDuration> _accumulations;

		STimePoint _startPause;
		SDuration _elapsedPause{0};
	};
}