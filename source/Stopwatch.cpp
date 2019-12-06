#include "Stopwatch.h"

#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>

#include "DateTime.h"
#include "Diagnostics.h"
#include "Exception.h"
#include "math/MathUtilities.h"
#define var const auto

namespace Jde
{
	//using NanoSecs = std::chrono::duration<size_t, std::chrono::nanoseconds::period>;
	using namespace std::chrono;
	//using std::chrono::duration_cast;

	SDuration Stopwatch::_minimumToLog = 1s;
	//map<string,SDuration> Stopwatch::_accumulations;

	Stopwatch::Stopwatch( std::string_view what, bool started )noexcept:
		_what{ what },
		_start{ started ? SClock::now() : STimePoint{} }
	{}

	Stopwatch::Stopwatch( Stopwatch* pParent, std::string_view what, string_view instance )noexcept:
		_what{ what },//what.length()==0 ? StopwatchNS::Context :
		_instance{instance},
		_start{ SClock::now() },
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
	string Stopwatch::Progress( uint index, uint total, std::string_view context, bool force/*=false*/ )const
	{
		if( index==0 )
			index = 1;
		var elapsed = Elapsed();
		std::ostringstream os;
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
				DBG0( os.str() );
			}
		}
		_previousProgressElapsed = elapsed;
		return result;
	}
	void Stopwatch::Output( string_view what, const SDuration& elapsed, bool logMemory )
	{
		if( logMemory )
			DBG( "{{}) time:  {} - {} Gigs", what, FormatSeconds(elapsed), Diagnostics::GetMemorySize()/std::pow(2,30) );
		else
			DBG( "({}) time:  {}", what, FormatSeconds(elapsed) );
	}
	void Stopwatch::Finish( bool /*remove=true*/ )
	{
		//if( remove )
			//StopwatchNS::Existing.erase( this );
		Finish( ""sv );
	}

	void Stopwatch::Finish( string_view description )
	{
		var elapsed = Elapsed();
		if( _pParent  )
		{
			auto pChild = _pParent->_children.emplace( _what, std::chrono::nanoseconds(0) ).first;
			pChild->second += elapsed;
		}
		if( elapsed-_previousProgressElapsed>_minimumToLog || _children.size()>0 )
		{
			Output( description.size() ? description : _instance.size() ? _instance : _what, elapsed, _logMemory );
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
		auto pChild = _children.emplace( sw._what, std::chrono::nanoseconds(0) ).first;
		pChild->second += sw.Elapsed();
	}
*/
/*	void Stopwatch::SetContext( const string& context )
	{ 
		StopwatchNS::Context=context; 
	}
*/
	std::string Stopwatch::FormatSeconds( const SDuration& duration )
	{
		double seconds = duration_cast<std::chrono::milliseconds>(duration).count()/1000.0; //;
		string format;
		if( seconds < 60.0 )
			format = fmt::format( "{:.1f}", seconds );
		else
		{
			var wholeSeconds = Math::URound(seconds);
			if( wholeSeconds < 60*60 )
				format = fmt::format( "{:0>2}:{:0>2}", (wholeSeconds/60), (wholeSeconds%60) );
			else if( wholeSeconds < 60*60*24 )
				format = fmt::format( "{:0>2}:{:0>2}:{:0>2}",  (wholeSeconds/(60*60)),  (wholeSeconds%(60*60)/60),  (wholeSeconds%60) );
			else
				format = fmt::format( "{}:{:0>2}:{:0>2}:{:0>2}", (wholeSeconds/(60*60*24)), (wholeSeconds%(60*60*24)/(60*60)), (wholeSeconds%(60*60)/60), (wholeSeconds%60) );
		}
		return format;
	}

	std::string Stopwatch::FormatCount( double count )
	{
		std::string fmt;
		if( count > 100000.0 )
			fmt = fmt::format( "{0,p1}M", count/=1000000.0 );
		else if( count > 1000.0 )
			fmt = fmt::format( "{0}k", count/=1000.0 );
		else
			fmt = fmt::format( "{0}", count );
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
			THROW( Exception(fmt::format("enum:  '{0}', not implemented.", int(type)).c_str()) );
		}
		return name;
	}
*/	
	void Stopwatch::UnPause()
	{
		if( _startPause.time_since_epoch().count() )
		{
			_elapsedPause += duration_cast<SDuration>( SClock::now() - _startPause );
			_startPause = STimePoint{};
		}
	}
	void Stopwatch::Pause()
	{
		if( !_startPause.time_since_epoch().count() )
			_startPause = SClock::now();
	}
}