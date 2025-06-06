﻿#include "DateTime.h"
#include <jde/framework/str.h>
#include "math/MathUtilities.h"

#define let const auto

namespace Jde
{
	using namespace std::chrono;

	static TimePoint _epoch{ DateTime(1970,1,1).GetTimePoint() };

	α Chrono::Min( TimePoint a, TimePoint b )ι->TimePoint{ return a.time_since_epoch()<b.time_since_epoch() ? a : b; }

	α Chrono::Epoch()ι->TimePoint{ return _epoch; };

	α Chrono::ToDuration( sv iso )ε->Duration{
		std::istringstream is{ string{iso} };
		if( is.get()!='P' )
			THROW( "Expected 'P' as first character." );
		bool parsingTime = false;
		Duration duration{ Duration::zero() };
		while( is.good() ){
			if( !parsingTime && (parsingTime = is.peek()=='T') ){
				is.get();
				continue;
			}
			double value;
			is >> value;
			let type = is.get();
			if( type=='Y' )
				duration += hours( Round(value*365.25*24) );
			else if( !parsingTime && type=='M' )
				duration += hours( Round(value*30*24) );
			else if( type=='D' )
				duration += hours( Round(value*24) );
			else if( type=='H' )
				duration += minutes( Round(value*60) );
			else if( type=='M' )
				duration += seconds( Round(value*60) );
			else if( type=='S' )
				duration += milliseconds( Round(value*1000) );
		}
		return duration;
	}

	DateTime::DateTime()ι:
		_time_point( system_clock::now() )
	{}

	DateTime::DateTime( const DateTime& other )ι
	{
		*this = other;
	}

	DateTime::DateTime( time_t time )ι:
		_time_point( system_clock::from_time_t(time) )
	{}

	DateTime::DateTime( TimePoint tp )ι:
		_time_point( tp )
	{}

	DateTime::DateTime( fs::file_time_type t )ι:
		_time_point( Chrono::ToClock<Clock,fs::file_time_type::clock>(t) )
	{}

	DateTime::DateTime( steady_clock::time_point time )ι:
		_time_point( Chrono::ToClock<Clock,steady_clock>(time) )
	{}


	DateTime::DateTime( uint16 year, uint8 month, uint8 day, uint8 hour, uint8 minute, uint8 second, Duration nanoFraction )ι{
		std::tm tm;
		tm.tm_year = year-1900;
		tm.tm_mon = month-1;
		tm.tm_mday = day;
		tm.tm_hour = hour;
		tm.tm_min = minute;
		tm.tm_sec = second;
#ifdef _MSC_VER
		let time = _mkgmtime( &tm );
#else
		let time = timegm( &tm );
#endif
		_time_point = Clock::from_time_t( time )+nanoFraction;
	}

	DateTime::DateTime( sv iso  )ε
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
		let time = _mkgmtime( &tm );
#else
		let time = timegm( &tm );
#endif
		_time_point = Clock::from_time_t( time );
	}

	α DateTime::DateDisplay()Ι->string
	{
		return Jde::format( "{:0>2}/{:0>2}/{:0>2}", Month(), Day(), Year()-2000 );
	}
	α DateTime::BeginingOfWeek()->DateTime
	{
		let now = time(nullptr);
		constexpr uint16 secondsPerDay = 60*60*24;
		let date = DateTime( now/secondsPerDay*secondsPerDay );
		let beginingOfWeek = date.TimeT() - date.Tm()->tm_wday*secondsPerDay;
		return DateTime( beginingOfWeek );
	}

	α DateTime::operator=(const DateTime& other)ι->DateTime&
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

	α DateTime::Nanos()Ι->uint32
	{
		return _time_point.time_since_epoch().count()%TimeSpan::NanosPerSecond;
	}

	α DateTime::TimeT()Ι->time_t
	{
		if( !_pTime )
			_pTime = mu<time_t>( system_clock::to_time_t(_time_point) );
		return *_pTime;
	}

	α DateTime::Tm()Ι->sp<std::tm>
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

	α DateTime::LocalTm()Ι->up<std::tm>
	{
		time_t time = TimeT();
		auto pLocal = mu<std::tm>();
#ifdef _MSC_VER
		_localtime64_s( pLocal.get(), &time );
#else
		localtime_r( &time, pLocal.get() );
#endif
		return pLocal;
	}

	α DateTime::TimeDisplay()Ι->string
	{
		return Jde::format( "{:0>2}:{:0>2}", Hour(), Minute() );
	}
	α DateTime::LocalTimeDisplay( bool seconds, bool milli )Ι->string
	{
		string y{ "null" };
		if( _time_point!=TP{} )
		{
			auto pLocal = LocalTm();
			string suffix{ seconds ? Jde::format(":{:0>2}", pLocal->tm_sec) : string{} };
			if( milli ){
				uint millisecs = _time_point.time_since_epoch().count()%1000;//gcc
				suffix = Jde::format( "{}.{:0>2}", suffix, millisecs );
			}
			y = Jde::format( "{:0>2}:{:0>2}{}", pLocal->tm_hour, pLocal->tm_min, suffix );
		}
		return y;
	}
	α DateTime::LocalDateDisplay()Ι->string
	{
		auto pLocal = LocalTm();
		return Jde::format( "{:0>2}/{:0>2}/{:0>2}", pLocal->tm_mon+1, pLocal->tm_mday, pLocal->tm_year-100 );
	}
	α DateTime::LocalDisplay( bool seconds, bool milli )Ι->string
	{
		return Jde::format( "{} {}", LocalDateDisplay(), LocalTimeDisplay(seconds, milli) );
	}
	α DateTime::ToDate( TimePoint time )ι->TimePoint
	{
		let secondsSinceEpoch = duration_cast<seconds>( time.time_since_epoch() ).count();
		constexpr uint secondsPerDay = duration_cast<seconds>( 24h ).count();
		let day = time - seconds( secondsSinceEpoch%secondsPerDay );
		return day;
	}
	α DateTime::ToIsoString()Ι->string
	{
		return ToIsoString( *Tm() );
	}
	α DateTime::ToIsoString(const tm& t)ι->string
	{
		return Jde::format( "{}-{:0>2}-{:0>2}T{:0>2}:{:0>2}:{:0>2}Z", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec ).c_str();
	}

	constexpr std::array<sv,12> months{ "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
	uint8 DateTime::ParseMonth( sv month )ε
	{
		let index = find( months.begin(), months.end(), Str::ToLower(string(month)) )-months.begin();
		THROW_IF( index>=(int)months.size(), "Could not parse month '{}'", month );
		return (uint8)index+1;
	}
	α DateTime::MonthAbbrev()Ι->string
	{
		let month = months[Month()-1];
		return string{ (char)std::toupper(month[0]),month[1],month[2] };
	}
}
namespace Jde
{
	α Timezone::TryGetGmtOffset( sv name, TimePoint utc, SL sl )ι->Duration
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
	α Timezone::GetGmtOffset( sv inputName, TimePoint utc, SL sl )ε->Duration
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
	α Timezone::EasternTimezoneDifference( TimePoint time, SL sl )ε->Duration
	{
		return GetGmtOffset( "America/New_York", time, sl );
	}
#endif
}