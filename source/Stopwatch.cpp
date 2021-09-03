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
	//using NanoSecs = duration<size_t, nanoseconds::period>;
	using namespace std::chrono;
	//using duration_cast;

	SDuration Stopwatch::_minimumToLog = 1s;
	//map<string,SDuration> Stopwatch::_accumulations;

	Stopwatch::Stopwatch( sv what, bool started )noexcept:
		_what{ what },
		_start{ started ? SClock::now() : STimePoint{} }
	{}

	Stopwatch::Stopwatch( Stopwatch* pParent, sv what, sv instance, bool started )noexcept:
		_what{ what },//what.length()==0 ? StopwatchNS::Context :
		_instance{instance},
		_start{ started ? SClock::now() : STimePoint{} },
		_pParent{ pParent },
		_logMemory{ false }
	{}

	Stopwatch::~Stopwatch()
	{
		if( !_finished )
			Finish();
	}

	SDuration Stopwatch::Elapsed()const
	{
		auto end = SClock::now();
		var asNano = duration_cast<SDuration>( end - _start - _elapsedPause );
		return asNano;
	}
	string Stopwatch::Progress( uint index, uint total, sv context, bool force/*=false*/ )const
	{
		if( index==0 )
			index = 1;
		var elapsed = Elapsed();
		ostringstream os;
		string result;
		if( force || elapsed-_previousProgressElapsed>_minimumToLog )
		{
			os.imbue( std::locale("") );
			//os << GetTypeName() << "(" << _what.c_str() << ")  ";
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
				DBG( "Progress={}"sv, os.str() );
			}
		}
		_previousProgressElapsed = elapsed;
		return result;
	}
	void Stopwatch::Output( sv what, const SDuration& elapsed, bool logMemory )
	{
		if( logMemory )
			DBG( "{{}) time:  {} - {} Gigs"sv, what, FormatSeconds(elapsed), IApplication::MemorySize()/std::pow(2,30) );
		else
			DBG( "({}) time:  {}"sv, what, FormatSeconds(elapsed) );
	}
	void Stopwatch::Finish( bool /*remove=true*/ )
	{
		//if( remove )
			//StopwatchNS::Existing.erase( this );
		Finish( ""sv );
	}

	void Stopwatch::Finish( sv description )
	{
		var elapsed = Elapsed();
		Pause();
		if( _pParent  )
			_pParent->_children.emplace( _what, nanoseconds(0) ).first->second += elapsed;
		var delta = elapsed-_previousProgressElapsed;
		if( delta>_minimumToLog || _children.size()>0 )
		{
			_previousProgressElapsed = elapsed;
			Output( description.size() ? description : _instance.size() ? _instance : _what, delta, _logMemory );
			for( var& child : _children )
			{
				if( child.second>_minimumToLog )
					Output( /*StopwatchTypes::Other,*/ child.first.c_str(), child.second, false );
			}
		}
		_finished=true;
	}

/*	void Stopwatch::UpdateChild( const Stopwatch& sw )
	{
		auto pChild = _children.emplace( sw._what, nanoseconds(0) ).first;
		pChild->second += sw.Elapsed();
	}
*/
/*	void Stopwatch::SetContext( str context )
	{
		StopwatchNS::Context=context;
	}
*/
	string Stopwatch::FormatSeconds( const SDuration& duration )
	{
		double seconds = duration_cast<milliseconds>(duration).count()/1000.0; //;
		string fmt;
		if( seconds < 60.0 )
			fmt = format( "{:.1f}"sv, seconds );
		else
		{
			var wholeSeconds = Math::URound(seconds);
			if( wholeSeconds < 60*60 )
				fmt = format( "{:0>2}:{:0>2}", (wholeSeconds/60), (wholeSeconds%60) );
			else if( wholeSeconds < 60*60*24 )
				fmt = format( "{:0>2}:{:0>2}:{:0>2}",  (wholeSeconds/(60*60)),  (wholeSeconds%(60*60)/60),  (wholeSeconds%60) );
			else
				fmt = format( "{}:{:0>2}:{:0>2}:{:0>2}", (wholeSeconds/(60*60*24)), (wholeSeconds%(60*60*24)/(60*60)), (wholeSeconds%(60*60)/60), (wholeSeconds%60) );
		}
		return fmt;
	}

	string Stopwatch::FormatCount( double count )
	{
		string fmt;
		if( count > 100000.0 )
			fmt = format( "{0}M"sv, count/=1000000.0 );
		else if( count > 1000.0 )
			fmt = format( "{0}k", count/=1000.0 );
		else
			fmt = format( "{0}", count );
		return fmt;
	}
/*
	const char* Stopwatch::GetTypeName(StopwatchTypes type)
	{
		const char* name="";
		switch( type )
		{
		case StopwatchTypes::Other:
			name = "Other";
			break;
		case StopwatchTypes::Calculate:
			name = "Calculate";
			break;
		case StopwatchTypes::ComputeCost:
			name = "ComputeCost";
			break;
		case StopwatchTypes::Convert:
			name = "Convert";
			break;
		case StopwatchTypes::Copy:
			name = "Copy";
			break;
		case StopwatchTypes::Create:
			name = "Create";
			break;
		case StopwatchTypes::GradientDescent:
			name = "GradientDescent";
			break;
		case StopwatchTypes::Prefix:
			name = "Prefix";
			break;
		case StopwatchTypes::ReadFile:
			name = "Read File";
			break;
		case StopwatchTypes::WriteFile:
			name = "Write File";
			break;
		case StopwatchTypes::ServerCall:
			name = "Server Call";
			break;
		default:
			THROW( Exception(format("enum:  '{0}', not implemented.", int(type)).c_str()) );
		}
		return name;
	}
*/
	void Stopwatch::UnPause()
	{
		if( _startPause!=STimePoint{} )
		{
			_elapsedPause += duration_cast<SDuration>( SClock::now() - _startPause );
			_startPause = STimePoint{};
		}
	}
	void Stopwatch::Pause()
	{
		if( _startPause==STimePoint{} )
			_startPause = SClock::now();
	}
}