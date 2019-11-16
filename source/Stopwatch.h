#pragma once
#include <chrono>
#include <map>
#include <memory>
#include <string>

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
	class JDE_NATIVE_VISIBILITY Stopwatch
	{
	public:
		Stopwatch( std::string_view what, bool started=true )noexcept;
		Stopwatch( Stopwatch* pParent, std::string_view what="", std::string_view instance="" )noexcept;
		virtual ~Stopwatch();

		void Finish( string_view description );
		void Finish( bool remove=true );
		string Progress( uint count, uint total=0 )const{ return Progress( count, total, "" ); }
		string Progress( uint count, uint total, std::string_view context, bool force=false )const;
		SDuration Elapsed()const;
		static std::string FormatSeconds( const SDuration& seconds );
		static std::string FormatCount( double total );
		void Pause();
		void UnPause();
		void Restart(){ _start = std::chrono::steady_clock::now(); }
	private:
		//void UpdateChild( const Stopwatch& sw );
		static void Output( std::string_view what, const SDuration& elapsed, bool logMemory );

		const std::string _what;
		const std::string _instance;
		std::chrono::steady_clock::time_point _start;
		
		Stopwatch* _pParent{nullptr};
		std::map<std::string,SDuration> _children;
		
		bool _finished{false};
		const bool _logMemory{false};
		static SDuration _minimumToLog;
		mutable SDuration _previousProgressElapsed{0};
		static map<string,SDuration> _accumulations;

		STimePoint _startPause;
		SDuration _elapsedPause{0};
	};

}