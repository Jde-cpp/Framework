#include "Stopwatch.h"

#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>

#include "DateTime.h"
#include "math/MathUtilities.h"
#include <jde/framework/process.h>
#define let const auto

namespace Jde{
	using namespace std::chrono;
	Stopwatch::SDuration Stopwatch::_minimumToLog = 1s;

	Stopwatch::Stopwatch( sv what, ELogTags tags, bool started, SL sl )ι:
		_what{ what },
		_start{ started ? steady_clock::now() : STimePoint{} },
		_tags{ tags },
		_sl{ sl }
	{}

	Stopwatch::Stopwatch( sv what, bool started, SL sl )ι:
		Stopwatch{ what, ELogTags::App, started, sl }
	{}

	Stopwatch::Stopwatch( Stopwatch* pParent, sv what, sv instance, bool started, SL sl )ι:
		_what{ what },
		_instance{instance},
		_start{ started ? steady_clock::now() : STimePoint{} },
		_pParent{ pParent },
		_logMemory{ false },
		_sl{ sl }
	{}

	Stopwatch::~Stopwatch()
	{
		if( !_finished )
			Finish();
	}

	α Stopwatch::Elapsed()Ι->Stopwatch::SDuration{
		auto end = steady_clock::now();
		let asNano = duration_cast<SDuration>( end - _start - _elapsedPause );
		return asNano;
	}
	α Stopwatch::Progress( uint index, uint total, sv context, bool force/*=false*/ )Ι->string{
		if( index==0 )
			index = 1;
		let elapsed = Elapsed();
		std::ostringstream os;
		string result;
		if( force || elapsed-_previousProgressElapsed>_minimumToLog ){
			os.imbue( std::locale("") );
			if( context.length()>0 )
				os << "[" << context << "]  ";
			os << std::fixed << index;
			double percentComplete=0.0;
			if( total!=0 ){
				percentComplete = double(index)/double(total);
				os << "/" << FormatCount(double(total)) << "(" << std::setprecision(1) << percentComplete*100.0 << "%)";
			}

			os << "  Seconds Spent:  " << std::setprecision(1) << FormatSeconds(elapsed);
			if( total!=0 ){
				let toGo = duration_cast<SDuration>( elapsed/percentComplete );
				os << "/" <<  FormatSeconds( toGo );
			}
			const double recordsPerSecond = double(index)/duration_cast<seconds>( elapsed ).count();
			if( recordsPerSecond> 1.0 )
				os << ".  per second:" << std::setprecision(1) << recordsPerSecond;
			else
				os << ".  seconds/record:" << std::setprecision(1) << 1/recordsPerSecond;
			if( force || elapsed-_previousProgressElapsed>_minimumToLog ){
				result = os.str();
				for( let& child : _children ){
					let& childElapsed = child.second;
					os << "(" << child.first << "=" << FormatSeconds(childElapsed/index) << ")";
				}
				Trace{ _tags, "Progress={}", os.str() };
			}
		}
		_previousProgressElapsed = elapsed;
		return result;
	}
	α Stopwatch::Output( sv what, const SDuration& elapsed, bool logMemory, SL sl )ι->void{
		if( logMemory )
			Trace( sl, _tags, "({}) time:  {} - {} Gigs", what, FormatSeconds(elapsed), IApplication::MemorySize()/std::pow(2,30) );
		else
			Trace( sl, _tags, "({}) time:  {}", what, FormatSeconds(elapsed) );
	}
	α Stopwatch::Finish( bool /*remove=true*/ )ι->void{
		Finish( ""sv );
	}

	α Stopwatch::Finish( sv description )ι->void{
		let elapsed = Elapsed();
		Pause();
		if( _pParent  )
			_pParent->_children.emplace( _what, nanoseconds(0) ).first->second += elapsed;
		let delta = elapsed-_previousProgressElapsed;
		if( delta>_minimumToLog || _children.size()>0 )
		{
			_previousProgressElapsed = elapsed;
			Output( description.size() ? description : _instance.size() ? _instance : _what, delta, _logMemory, _sl );
			for( let& child : _children ){
				if( child.second>_minimumToLog )
					Output( /*StopwatchTypes::Other,*/ child.first.c_str(), child.second, false, _sl );
			}
		}
		_finished=true;
	}

	string Stopwatch::FormatSeconds( const SDuration& duration )ι{
		double seconds = duration_cast<milliseconds>(duration).count()/1000.0; //;
		string fmt;
		if( seconds < 60.0 )
			fmt = Jde::format( "{:.1f}"sv, seconds );
		else{
			let wholeSeconds = Round( seconds );
			if( wholeSeconds < 60*60 )
				fmt = Jde::format( "{:0>2}:{:0>2}", (wholeSeconds/60), (wholeSeconds%60) );
			else if( wholeSeconds < 60*60*24 )
				fmt = Jde::format( "{:0>2}:{:0>2}:{:0>2}",  (wholeSeconds/(60*60)),  (wholeSeconds%(60*60)/60),  (wholeSeconds%60) );
			else
				fmt = Jde::format( "{}:{:0>2}:{:0>2}:{:0>2}", (wholeSeconds/(60*60*24)), (wholeSeconds%(60*60*24)/(60*60)), (wholeSeconds%(60*60)/60), (wholeSeconds%60) );
		}
		return fmt;
	}

	string Stopwatch::FormatCount( double count )ι{
		string fmt;
		if( count > 100000.0 )
			fmt = Jde::format( "{0}M"sv, count/=1000000.0 );
		else if( count > 1000.0 )
			fmt = Jde::format( "{0}k", count/=1000.0 );
		else
			fmt = Jde::format( "{0}", count );
		return fmt;
	}

	α Stopwatch::UnPause()ι->void{
		if( _startPause!=STimePoint{} ){
			_elapsedPause += duration_cast<SDuration>( steady_clock::now() - _startPause );
			_startPause = STimePoint{};
		}
	}
	α Stopwatch::Pause()ι->void{
		if( _startPause==STimePoint{} )
			_startPause = steady_clock::now();
	}
}