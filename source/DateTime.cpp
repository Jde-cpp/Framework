#include "DateTime.h"
//#include <chrono>
#include <jde/Str.h>
#include "math/MathUtilities.h"

#define var const auto

namespace Jde
{
	using namespace std::chrono;

	static TimePoint _epoch{ DateTime(1970,1,1).GetTimePoint() };

	α Chrono::Min( TimePoint a, TimePoint b )noexcept->TimePoint{ return a.time_since_epoch()<b.time_since_epoch() ? a : b; }

	α Chrono::Epoch()noexcept->TimePoint{ return _epoch; };

	α Chrono::ToDuration( sv iso )noexcept(false)->Duration //P3Y6M4DT12H30M5S
	{
		std::istringstream is{ string{iso} };
		if( is.get()!='P' )
			THROW( "Expected 'P' as first character." );
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

	α Chrono::ToString( Duration d )noexcept->string
	{
		ostringstream os;
		os << 'P';
		#define output(period,suffix) if( d>=period{1} || d<=period{-1} ){ os << duration_cast<period>(d).count() << suffix; d%=period{1}; }
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
#ifdef _MSC_VER
		var time = _mkgmtime( &tm );
#else
		var time = timegm( &tm );
#endif
		_time_point = Clock::from_time_t( time )+nanoFraction;
	}

	DateTime::DateTime( sv iso  )noexcept(false)
	{
		THROW_IF( iso.size()<19, "ISO Date '{}' expected at least 19 characters vs '{}'", iso, iso.size() );
		std::tm tm;
		tm.tm_year = stoi( string(iso.substr(0,4)) )-1900;
		tm.tm_mon = stoi( string(iso.substr(5,2)) )-1;
		tm.tm_mday = stoi( string(iso.substr(8,2)) );
		tm.tm_hour = stoi( string(iso.substr(11,2)) );
		tm.tm_min = stoi( string(iso.substr(14,2)) );
		tm.tm_sec = stoi( string(iso.substr(17,2)) );
#ifdef _MSC_VER
		var time = _mkgmtime( &tm );
#else
		var time = timegm( &tm );
#endif
		_time_point = Clock::from_time_t( time );
	}

	α DateTime::DateDisplay()const noexcept->string
	{
		return format( "{:0>2}/{:0>2}/{:0>2}", Month(), Day(), Year()-2000 );
	}
	α DateTime::BeginingOfWeek()->DateTime
	{
		var now = time(nullptr);
		constexpr uint16 secondsPerDay = 60*60*24;
		var date = DateTime( now/secondsPerDay*secondsPerDay );
		var beginingOfWeek = date.TimeT() - date.Tm()->tm_wday*secondsPerDay;
		return DateTime( beginingOfWeek );
	}

	α DateTime::operator=(const DateTime& other)noexcept->DateTime&
	{
		_time_point = other._time_point;
		_pTime = nullptr;
		_pTm = nullptr;
		return *this;
	}

	α DateTime::operator+=( const Duration& timeSpan )->DateTime&
	{
		_time_point += timeSpan;
		_pTime = nullptr;
		_pTm = nullptr;
		return *this;
	}
	α DateTime::operator+( const Duration& timeSpan )const->DateTime
	{
		DateTime result{*this};
		result += timeSpan;
		return result;
	}

	α DateTime::Nanos()const noexcept->uint32
	{
		return _time_point.time_since_epoch().count()%TimeSpan::NanosPerSecond;
	}

	α DateTime::TimeT()const noexcept->time_t
	{
		if( !_pTime )
			_pTime = make_unique<time_t>( system_clock::to_time_t(_time_point) );
		return *_pTime;
	}

	α DateTime::Tm()const noexcept->sp<std::tm>
	{
		if( !_pTm )
		{
			time_t time = TimeT();
			_pTm = up<std::tm>( new std::tm() );
#ifdef _MSC_VER
			gmtime_s( _pTm.get(), &time );
#else
			gmtime_r( &time, _pTm.get() );
#endif
		}
		return _pTm;
	}

	α DateTime::LocalTm()const noexcept->up<std::tm>
	{
		time_t time = TimeT();
		auto pLocal = make_unique<std::tm>();
#ifdef _MSC_VER
		_localtime64_s( pLocal.get(), &time );
#else
		localtime_r( &time, pLocal.get() );
#endif
		return pLocal;
	}

	α DateTime::TimeDisplay()const noexcept->string
	{
		return fmt::format( "{:0>2}:{:0>2}", Hour(), Minute() );
	}
	α DateTime::LocalTimeDisplay()const noexcept->string
	{
		auto pLocal = LocalTm();
		return fmt::format( "{:0>2}:{:0>2}", pLocal->tm_hour, pLocal->tm_min );
	}
	α DateTime::LocalDateDisplay()const noexcept->string
	{
		auto pLocal = LocalTm();
		return fmt::format( "{:0>2}/{:0>2}/{:0>2}", pLocal->tm_mon+1, pLocal->tm_mday, pLocal->tm_year-100 );
	}
	α DateTime::LocalDisplay()const noexcept->string
	{
		return fmt::format( "{} {}", LocalDateDisplay(), LocalTimeDisplay() );
	}
	α DateTime::ToDate( TimePoint time )noexcept->TimePoint
	{
		var secondsSinceEpoch = duration_cast<seconds>( time.time_since_epoch() ).count();
		constexpr uint secondsPerDay = duration_cast<seconds>( 24h ).count();
		var day = time - seconds( secondsSinceEpoch%secondsPerDay );
		return day;
	}
	α DateTime::ToIsoString()const noexcept->string
	{
		return ToIsoString( *Tm() );
	}
	α DateTime::ToIsoString(const tm& t)noexcept->string
	{
		return fmt::format( "{}-{:0>2}-{:0>2}T{:0>2}:{:0>2}:{:0>2}Z", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec ).c_str();
	}

	constexpr std::array<sv,12> months{ "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
	uint8 DateTime::ParseMonth( sv month )noexcept(false)
	{
		var index = find( months.begin(), months.end(), Str::ToLower(string(month)) )-months.begin();
		THROW_IF( index>=(int)months.size(), "Could not parse month '{}'", month );
		return (uint8)index+1;
	}
	α DateTime::MonthAbbrev()const noexcept->string
	{
		var month = months[Month()-1];
		return string{ (char)std::toupper(month[0]),month[1],month[2] };
	}
}
namespace Jde
{
	α Timezone::TryGetGmtOffset( sv name, TimePoint utc, SL sl )noexcept->Duration
	{
		try
		{
			return GetGmtOffset( name, utc, sl );
		}
		catch( const IException& )
		{}
		return Duration{};
	}
#ifdef _MSC_VER
	α Timezone::GetGmtOffset( sv inputName, TimePoint utc, SL sl )noexcept(false)->Duration 
	{
/*		CIString ciName{ inputName };
		if( ciName=="EST (Eastern Standard Time)"sv || ciName=="US/Eastern"sv )
			ciName = "Eastern Standard Time"sv;
		else if( ciName=="MET (Middle Europe Time)"sv || ciName=="MET"sv )
			ciName = "W. Europe Standard Time"sv;*/
		try
		{
			return std::chrono::zoned_time{ inputName, utc }.get_info().offset;
		}
		catch( std::exception& e )
		{
			throw Exception( sl, move(e), "Could not find time zone '{}'", inputName );
		}
	}
	α Timezone::EasternTimezoneDifference( TimePoint time, SL sl )noexcept(false)->Duration
	{
		return GetGmtOffset( "America/New_York", time, sl );
	}
#endif
}
