#include "Stopwatch.h"

#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>

#include "DateTime.h"
#include <jde/Exception.h>
#include "math/MathUtilities.h"
#include <jde/App.h>
#define var const auto

namespace Jde
{
	using namespace std::chrono;
	static var _logTag{ Logging::Tag("stopwatch") };
	Stopwatch::SDuration Stopwatch::_minimumToLog = 1s;

	Stopwatch::Stopwatch( sv what, bool started, SL sl )ι:
		_what{ what },
		_start{ started ? SClock::now() : STimePoint{} },
		_sl{ sl }
	{}

	Stopwatch::Stopwatch( Stopwatch* pParent, sv what, sv instance, bool started, SL sl )ι:
		_what{ what },
		_instance{instance},
		_start{ started ? SClock::now() : STimePoint{} },
		_pParent{ pParent },
		_logMemory{ false },
		_sl{ sl }
	{}

	Stopwatch::~Stopwatch()
	{
		if( !_finished )
			Finish();
	}

	α Stopwatch::Elapsed()Ι->Stopwatch::SDuration
	{
		auto end = SClock::now();
		var asNano = duration_cast<SDuration>( end - _start - _elapsedPause );
		return asNano;
	}
	α Stopwatch::Progress( uint index, uint total, sv context, bool force/*=false*/ )Ι->string
	{
		if( index==0 )
			index = 1;
		var elapsed = Elapsed();
		ostringstream os;
		string result;
		if( force || elapsed-_previousProgressElapsed>_minimumToLog )
		{
			os.imbue( std::locale("") );
			if( context.length()>0 )
				os << "[" << context << "]  ";
			os << std::fixed << index;
			double percentComplete=0.0;
			if( total!=0 )
			{
				percentComplete = double(index)/double(total);
				os << "/" << FormatCount(double(total)) << "(" << std::setprecision(1) << percentComplete*100.0 << "%)";
			}

			os << "  Seconds Spent:  " << std::setprecision(1) << FormatSeconds(elapsed);
			if( total!=0 )
			{
				var toGo = duration_cast<SDuration>( elapsed/percentComplete );
				os << "/" <<  FormatSeconds( toGo );
			}
			const double recordsPerSecond = double(index)/duration_cast<seconds>( elapsed ).count();
			if( recordsPerSecond> 1.0 )
				os << ".  per second:" << std::setprecision(1) << recordsPerSecond;
			else
				os << ".  seconds/record:" << std::setprecision(1) << 1/recordsPerSecond;
			if( force || elapsed-_previousProgressElapsed>_minimumToLog )
			{
				result = os.str();
				for( var& child : _children )
				{
					var& childElapsed = child.second;
					os << "(" << child.first << "=" << FormatSeconds(childElapsed/index) << ")";
				}
				TRACE( "Progress={}"sv, os.str() );
			}
		}
		_previousProgressElapsed = elapsed;
		return result;
	}
	α Stopwatch::Output( sv what, const SDuration& elapsed, bool logMemory, SL sl )ι->void
	{
		if( logMemory )
			DBGSL( "{{}) time:  {} - {} Gigs", what, FormatSeconds(elapsed), IApplication::MemorySize()/std::pow(2,30) );
		else
			DBGSL( "({}) time:  {}", what, FormatSeconds(elapsed) );
	}
	α Stopwatch::Finish( bool /*remove=true*/ )ι->void
	{
		Finish( ""sv );
	}

	α Stopwatch::Finish( sv description )ι->void
	{
		var elapsed = Elapsed();
		Pause();
		if( _pParent  )
			_pParent->_children.emplace( _what, nanoseconds(0) ).first->second += elapsed;
		var delta = elapsed-_previousProgressElapsed;
		if( delta>_minimumToLog || _children.size()>0 )
		{
			_previousProgressElapsed = elapsed;
			Output( description.size() ? description : _instance.size() ? _instance : _what, delta, _logMemory, _sl );
			for( var& child : _children )
			{
				if( child.second>_minimumToLog )
					Output( /*StopwatchTypes::Other,*/ child.first.c_str(), child.second, false, _sl );
			}
		}
		_finished=true;
	}

	string Stopwatch::FormatSeconds( const SDuration& duration )ι
	{
		double seconds = duration_cast<milliseconds>(duration).count()/1000.0; //;
		string fmt;
		if( seconds < 60.0 )
			fmt = Jde::format( "{:.1f}"sv, seconds );
		else
		{
			var wholeSeconds = Round( seconds );
			if( wholeSeconds < 60*60 )
				fmt = Jde::format( "{:0>2}:{:0>2}", (wholeSeconds/60), (wholeSeconds%60) );
			else if( wholeSeconds < 60*60*24 )
				fmt = Jde::format( "{:0>2}:{:0>2}:{:0>2}",  (wholeSeconds/(60*60)),  (wholeSeconds%(60*60)/60),  (wholeSeconds%60) );
			else
				fmt = Jde::format( "{}:{:0>2}:{:0>2}:{:0>2}", (wholeSeconds/(60*60*24)), (wholeSeconds%(60*60*24)/(60*60)), (wholeSeconds%(60*60)/60), (wholeSeconds%60) );
		}
		return fmt;
	}

	string Stopwatch::FormatCount( double count )ι
	{
		string fmt;
		if( count > 100000.0 )
			fmt = Jde::format( "{0}M"sv, count/=1000000.0 );
		else if( count > 1000.0 )
			fmt = Jde::format( "{0}k", count/=1000.0 );
		else
			fmt = Jde::format( "{0}", count );
		return fmt;
	}

	α Stopwatch::UnPause()ι->void
	{
		if( _startPause!=STimePoint{} )
		{
			_elapsedPause += duration_cast<SDuration>( SClock::now() - _startPause );
			_startPause = STimePoint{};
		}
	}
	α Stopwatch::Pause()ι->void
	{
		if( _startPause==STimePoint{} )
			_startPause = SClock::now();
	}
}