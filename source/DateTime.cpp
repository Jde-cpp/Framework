#include "DateTime.h"
//#include <chrono>
#include <jde/Str.h>
#include "math/MathUtilities.h"

#define var const auto

namespace Jde
{
	using namespace std::chrono;
	const TimePoint& Chrono::Min( const TimePoint& a, const TimePoint& b )noexcept{ return a.time_since_epoch()<b.time_since_epoch() ? a : b; }
	namespace Chrono
	{
		TimePoint _epoch{ DateTime(1970,1,1).GetTimePoint() };
		TimePoint Epoch()noexcept{ return _epoch; };

		Duration ToDuration( sv iso )noexcept(false)//P3Y6M4DT12H30M5S
		{
			std::istringstream is{ string{iso} };
			if( is.get()!='P' )
				THROW( Exception("Expected 'P' as first character.") );
			bool parsingTime = false;
			Duration duration{ Duration::zero() };
			while( is.good() )
			{
				if( !parsingTime && (parsingTime = is.peek()=='T') )
				{
					is.get();
					continue;
				}
				double value;
				is >> value;
				var type = is.get();
				if( type=='Y' )
					duration += hours( Math::URound(value*365.25*24) );
				else if( !parsingTime && type=='M' )
					duration += hours( Math::URound(value*30*24) );
				else if( type=='D' )
					duration += hours( Math::URound(value*24) );
				else if( type=='H' )
					duration += minutes( Math::URound(value*60) );
				else if( type=='M' )
					duration += seconds( Math::URound(value*60) );
				else if( type=='S' )
					duration += milliseconds( Math::URound(value*1000) );
			}
			return duration;
		}
		string ToString( Duration d )noexcept
		{
			ostringstream os;
			os << 'P';
			#define output(period,suffix) if( d>=period{1} || d<=period{-1} ){ os << duration_cast<period>(d).count() << suffix; d%=period{1}; }
			/*auto output = [&d,&os]<T>( auto period, sv suffix )
			{
				if( d>period )
				{
					os << duration_cast<years>(d).count() << suffix;
					d%=period;
				}
			};*/
#ifdef _MSC_VER
			var year = hours(24 * 365);
			if( d >= year || d <= -year )
			{
				os << duration_cast<hours>(d).count()/year.count() << "Y";
				d %= year;
			}
			var month = hours(24 * 30);
			if( d >= month || d <= -month )
			{
				os << duration_cast<hours>(d).count() / month.count() << "M";
				d %= month;
			}
			var days = hours(24);
			if( d >= days || d <= -days )
			{
				os << duration_cast<hours>(d).count() / days.count() << "M";
				d %= days;
			}
#else
			output( years, "Y" );
			output( months, "M" );
			output( days, "D" );
#endif
			if( d!=Duration::zero() )
			{
				os << "T";
				output( hours, "H" );
				output( minutes, "M" );
				output( seconds, "S" );
				if( d!=Duration::zero() )
					os << duration_cast<milliseconds>(d).count();
			}
			return os.str();
		}
	}

#ifndef __cplusplus
	TimePoint::TimePoint( const TimePoint& tp )noexcept:
		base{ tp },
		_text{ DateTime(tp).ToIsoString() }
	{}
	constexpr TimePoint& TimePoint::operator+=( const DurationType& x )noexcept
	{
		base::operator+=(x);
		_text=ToIsoString(base);
		return *this;
	}
#endif
	DateTime::DateTime()noexcept:
		_time_point( system_clock::now() )
	{}

	DateTime::DateTime( const DateTime& other )noexcept
	{
		*this = other;
	}

	DateTime::DateTime( time_t time )noexcept:
		_time_point( system_clock::from_time_t(time) )
	{}

	DateTime::DateTime( TimePoint tp )noexcept:
		_time_point( tp )
	{}

	DateTime::DateTime( fs::file_time_type t )noexcept:
		_time_point( Chrono::ToClock<Clock,fs::file_time_type::clock>(t) )
	{}

	DateTime::DateTime( uint16 year, uint8 month, uint8 day, uint8 hour, uint8 minute, uint8 second, Duration nanoFraction )noexcept
	{
		std::tm tm;
		tm.tm_year = year-1900;
		tm.tm_mon = month-1;
		tm.tm_mday = day;
		tm.tm_hour = hour;
		tm.tm_min = minute;
		tm.tm_sec = second;
#if _WINDOWS
		var time = _mkgmtime( &tm );
#else
		var time = timegm( &tm );
#endif
		_time_point = Clock::from_time_t( time )+nanoFraction;
	}

	DateTime::DateTime( sv iso  )noexcept(false)
	{
		if( iso.size()<19 )
			THROW( RuntimeException("ISO Date '{}' expected at least 19 characters vs '{}'", iso, iso.size()) );

		std::tm tm;
		tm.tm_year = stoi( string(iso.substr(0,4)) )-1900;
		tm.tm_mon = stoi( string(iso.substr(5,2)) )-1;
		tm.tm_mday = stoi( string(iso.substr(8,2)) );
		tm.tm_hour = stoi( string(iso.substr(11,2)) );
		tm.tm_min = stoi( string(iso.substr(14,2)) );
		tm.tm_sec = stoi( string(iso.substr(17,2)) );
#if _WINDOWS
		var time = _mkgmtime( &tm );
#else
		var time = timegm( &tm );
#endif
		_time_point = Clock::from_time_t( time );
	}

	std::string DateTime::DateDisplay()const noexcept
	{
		return format( "{:0>2}/{:0>2}/{:0>2}", Month(), Day(), Year()-2000 );
	}
	DateTime DateTime::BeginingOfWeek()
	{
		var now = time(nullptr);
		constexpr uint16 secondsPerDay = 60*60*24;
		var date = DateTime( now/secondsPerDay*secondsPerDay );
		var beginingOfWeek = date.TimeT() - date.Tm()->tm_wday*secondsPerDay;
		return DateTime( beginingOfWeek );
	}

	DateTime& DateTime::operator=(const DateTime& other)noexcept
	{
		_time_point = other._time_point;
		_pTime = nullptr;
		_pTm = nullptr;
		return *this;
	}

	DateTime& DateTime::operator+=( const Duration& timeSpan )
	{
		_time_point += timeSpan;
		_pTime = nullptr;
		_pTm = nullptr;
		return *this;
	}
	DateTime DateTime::operator+( const Duration& timeSpan )const
	{
		DateTime result{*this};
		result += timeSpan;
		return result;
	}

	uint32 DateTime::Nanos()const noexcept
	{
		return _time_point.time_since_epoch().count()%Chrono::TimeSpan::NanosPerSecond;
	}

	time_t DateTime::TimeT()const noexcept
	{
		if( !_pTime )
			_pTime = make_unique<time_t>( system_clock::to_time_t(_time_point) );
		return *_pTime;
	}

	shared_ptr<std::tm> DateTime::Tm()const noexcept
	{
		if( !_pTm )
		{
			time_t time = TimeT();
			_pTm = unique_ptr<std::tm>( new std::tm() );
#if _WINDOWS
			gmtime_s( _pTm.get(), &time );
#else
			gmtime_r( &time, _pTm.get() );
#endif
		}
		return _pTm;
	}

	unique_ptr<std::tm> DateTime::LocalTm()const noexcept
	{
		time_t time = TimeT();
		auto pLocal = make_unique<std::tm>();
#ifdef _WINDOWS
		localtime_s( pLocal.get(), &time );
#else
		localtime_r( &time, pLocal.get() );
#endif
		return pLocal;
	}

	std::string DateTime::TimeDisplay()const noexcept
	{
		return fmt::format( "{:0>2}:{:0>2}", Hour(), Minute() );
	}
	std::string DateTime::LocalTimeDisplay()const noexcept
	{
		auto pLocal = LocalTm();
		return fmt::format( "{:0>2}:{:0>2}", pLocal->tm_hour, pLocal->tm_min );
	}
	std::string DateTime::LocalDateDisplay()const noexcept
	{
		auto pLocal = LocalTm();
		return fmt::format( "{:0>2}/{:0>2}/{:0>2}", pLocal->tm_mon+1, pLocal->tm_mday, pLocal->tm_year-100 );
	}
	std::string DateTime::LocalDisplay()const noexcept
	{
		return fmt::format( "{} {}", LocalDateDisplay(), LocalTimeDisplay() );
	}
	TimePoint DateTime::ToDate( const TimePoint& time )noexcept
	{
		var secondsSinceEpoch = duration_cast<seconds>( time.time_since_epoch() ).count();
		constexpr uint secondsPerDay = duration_cast<seconds>( 24h ).count();
		var day = time - seconds( secondsSinceEpoch%secondsPerDay );
		return day;
	}
	std::string DateTime::ToIsoString()const noexcept
	{
		return ToIsoString( *Tm() );
	}
	std::string DateTime::ToIsoString(const tm& t)noexcept
	{
		return fmt::format( "{}-{:0>2}-{:0>2}T{:0>2}:{:0>2}:{:0>2}Z", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec ).c_str();
	}

	constexpr std::array<sv,12> months{ "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
	uint DateTime::ParseMonth( sv month )noexcept(false)
	{
		var index = find( months.begin(), months.end(), Str::ToLower(string(month)) )-months.begin();
		if( index>=(int)months.size() )
			THROW( ArgumentException("Could not parse month '{}'", month) );
		return index+1;
	}
	string DateTime::MonthAbbrev()const noexcept
	{
		var month = months[Month()-1];
		return string{ (char)std::toupper(month[0]),month[1],month[2] };
	}

	// inline TimePoint ToTimePoint( fs::file_time_type fileTime )noexcept
	// {
	// 	var fileTimeT = fs::file_time_type::clock::to_time_t( fileTime );
	// 	var fractional = fileTime-fs::file_time_type::clock::from_time_t( fileTimeT );
	// 	TimePoint point = Clock::from_time_t( fileTimeT );
	// 	Duration d = milliseconds( duration_cast<milliseconds>(fractional) );
	// 	point += d;
	// 	return point;
	// }

	namespace Timezone
	{
		Duration TryGetGmtOffset( sv name, TimePoint utc )noexcept
		{
			try
			{
				return GetGmtOffset( name, utc );
			}
			catch( const Exception& )
			{}
			return Duration{};
		}
	}

	std::ostream& operator<<( std::ostream &os, const Jde::DateTime& obj )noexcept
	{
		os << obj.ToIsoString( *obj.LocalTm() );
		return os;
	}
}
std::ostream& operator<<( std::ostream &os, const std::chrono::system_clock::time_point& obj )noexcept
{
	os << Jde::DateTime(obj).ToIsoString();
	return os;
}