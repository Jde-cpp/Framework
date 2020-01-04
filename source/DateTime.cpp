#include "DateTime.h"
//#include <spdlog/fmt/ostr.h>
#include "StringUtilities.h"
#include "math/MathUtilities.h"
#define var const auto



namespace Jde
{
	namespace Chrono
	{
		TimePoint _epoch{ DateTime(1970,1,1).GetTimePoint() };
		TimePoint Epoch()noexcept{ return _epoch; };
	}

	using namespace std::chrono;

	DateTime::DateTime()noexcept:
		_time_point( std::chrono::system_clock::now() )
	{}

	DateTime::DateTime( const DateTime& other )noexcept
	{
		*this = other;
	}

	DateTime::DateTime( time_t time )noexcept:
		_time_point( std::chrono::system_clock::from_time_t(time) )
	{}

	DateTime::DateTime( const TimePoint& tp )noexcept:
		_time_point( tp )
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

	DateTime::DateTime( string_view iso  )noexcept(false)
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
		return fmt::format("{:0>2}/{:0>2}/{:0>2}", Month(), Day(), Year()-2000);
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
/*	DateTime& DateTime::operator+=( const TimeSpan& timeSpan )
	{
		_time_point += timeSpan.Ticks();//_time_point + std::chrono::nanoseconds(8);
		_pTime = nullptr;
		_pTm = nullptr;
		return *this;
	}*/
	DateTime& DateTime::operator+=( const Duration& timeSpan )
	{
		_time_point += timeSpan;//_time_point + std::chrono::nanoseconds(8);
		_pTime = nullptr;
		_pTm = nullptr;
		return *this;
	}
	DateTime DateTime::operator+( const Duration& timeSpan )const
	{
		DateTime result{*this};
		result += timeSpan;//_time_point + std::chrono::nanoseconds(8);
		return result;
	}
/*	DateTime DateTime::operator+( const TimeSpan& timeSpan )const
	{
		DateTime result{*this};
		result+=timeSpan;
		return result;
	}
	DateTime DateTime::operator-( const TimeSpan& timeSpan )const
	{
		DateTime result{*this};
		result-=timeSpan;
		return result;
	}
	DateTime& DateTime::operator-=( const TimeSpan& timeSpan )
	{
		*this+=-timeSpan;
		return *this;
	}
*/
	uint32 DateTime::Nanos()const noexcept
	{
		return _time_point.time_since_epoch().count()%Chrono::TimeSpan::NanosPerSecond;
	}

	time_t DateTime::TimeT()const noexcept
	{
		if( !_pTime )
			_pTime = make_unique<time_t>( std::chrono::system_clock::to_time_t(_time_point) );
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
		//return std::move(pLocal);
	}

	/*time_t DateTime::LocalDate()const noexcept
	{
		var pLocal = LocalTime();
		return mktime( pLocal.get() )/(60*60*24);
	}
	*/
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
//		DBG( "Time {} switched to Date {}.", DateTime(time).ToIsoString(), DateTime(day).ToIsoString() );
		return day;
		//MarketOpenTime = DateTime::ToDate( time ); static_cast<uint32>( time - %TimeSpan::SecondsPerDay + 9*TimeSpan::SecondsPerHour+30*TimeSpan::SecondsPerMinute - timeZoneDiff );
	}
	std::string DateTime::ToIsoString()const noexcept
	{
		return ToIsoString( *Tm() );
	}
	std::string DateTime::ToIsoString(const tm& t)noexcept
	{
		return fmt::format( "{}-{:0>2}-{:0>2}T{:0>2}:{:0>2}:{:0>2}Z", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec ).c_str();
	}

	constexpr std::array<string_view,12> months{ "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
	uint DateTime::ParseMonth( string_view month )noexcept(false)
	{
		var index = find( months.begin(), months.end(), StringUtilities::ToLower(month) )-months.begin();
		if( index>=(int)months.size() )
			THROW( ArgumentException("Could not parse month '{}'", month) );
		return index+1;
	}
	string DateTime::MonthAbbrev()const noexcept
	{
		var month = months[Month()-1];
		return string{ (char)std::toupper(month[0]),month[1],month[2] };
	}

	namespace Timezone
	{
		Duration TryGetGmtOffset( string_view name, TimePoint utc )noexcept
		{
			try
			{
				return GetGmtOffset( name, utc );
			}
			catch(Exception)
			{}
			return Duration{};
		}
	}
/*		Duration EasternTimeZoneDifference( const TimePoint& time )noexcept
		{
#ifdef _WINDOWS
			var timet = DateTime(time).TimeT();
			struct tm tm;
			gmtime_s( &tm, &timet );
			tm.tm_isdst = -1;
			var gmt = mktime( &tm );
			return seconds( Math::URound(difftime(timet, gmt)) );
#else
			var dateValue = Clock::to_time_t( time ); // UTC
			tm gmt;
			gmtime_r( &dateValue, &gmt ); // further convert to GMT presuming now in local
			mktime( &gmt );//sets time zone to 'edt'
			if( string(gmt.tm_zone)!=string("EDT") && string(gmt.tm_zone)!=string("EST") )
				THROW( Exception("Need to implement for {} timezone.", gmt.tm_zone) );
			return seconds( gmt.tm_gmtoff );
#endif
		}
	}
	*/
	std::ostream& operator<<( std::ostream &os, const Jde::DateTime& obj )noexcept
	{
		os << obj.ToIsoString( *obj.LocalTm() );
//		os << fmt::format("{}-{:0>2}-{:0>2}T{:0>2}:{:0>2}:{:0>2}", pLocalTime->tm_year+1900, pLocalTime->tm_mon+1, pLocalTime->tm_mday, pLocalTime->tm_hour, pLocalTime->tm_min, pLocalTime->tm_sec).c_str();
		return os;
	}
}
std::ostream& operator<<( std::ostream &os, const std::chrono::system_clock::time_point& obj )noexcept
{
	os << Jde::DateTime(obj).ToIsoString();
	return os;
}