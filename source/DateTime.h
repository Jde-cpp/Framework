#pragma once
#ifndef DATE_TIME_H //gcc precompiled headers
#define DATE_TIME_H
#include <chrono>
#include <jde/TypeDefs.h>
#include <jde/Exports.h>

namespace Jde::Chrono
{
	using namespace std::chrono;
	Γ α Epoch()ι->TimePoint;
	Ξ MillisecondsSinceEpoch( TimePoint time=Clock::now() )ι->uint{ return duration_cast<std::chrono::milliseconds>( time-Epoch() ).count(); }
	Γ α Min( TimePoint a, TimePoint b )ι->TimePoint;

	Ξ Date( TimePoint time )ι->TimePoint{ return Clock::from_time_t( Clock::to_time_t(time)/(60*60*24)*(60*60*24) ); }
	Ξ Time( TimePoint time )ι->Duration{ return time-Date(time); }

	Ξ FromDays( DayIndex days )ι->TimePoint{ return Epoch()+days*24h; }
	Ξ ToDays( TimePoint time )ι->DayIndex{ return duration_cast<std::chrono::hours>( time-Epoch()).count()/24; }
	Ξ ToDays( time_t time )ι->DayIndex{ return ToDays(Clock::from_time_t(time)); }

	Γ α ToDuration( sv iso )ε->Duration;
	template<class T=Duration>
	α ToString( T duration )ι->string;
}
namespace Jde
{
	enum class DayOfWeek : uint8{ Sunday=0, Monday=1, Tuesday=2, Wednesday=3, Thursday=4, Friday=5, Saturday=6 };

	struct Γ DateTime
	{
		DateTime()ι;
		DateTime( const DateTime& other )ι;
		DateTime( time_t time )ι;
		DateTime( uint16 year, uint8 month, uint8 day, uint8 hour=0, uint8 minute=0, uint8 second=0, Duration nanoFraction=Duration{0} )ι;
		DateTime( sv iso )ε;
		DateTime( TimePoint tp )ι;
		DateTime( fs::file_time_type time )ι;
		Ω BeginingOfWeek()->DateTime;
		DateTime& operator=(const DateTime& other)ι;
		bool operator==(const DateTime& other)Ι{return _time_point==other._time_point; }
		bool operator<(const DateTime& other)Ι{return _time_point<other._time_point;}
		DateTime operator+( const Duration& timeSpan )const;
		DateTime& operator+=( const Duration& timeSpan );

		Ω DayOfWk( TimePoint time )ι{ return DateTime{time}.DayOfWk();}
		DayOfWeek DayOfWk()Ι{ return (DayOfWeek)(Tm()->tm_wday);}
		uint32 Nanos()Ι;
		uint_fast8_t Second()Ι{ return static_cast<uint_fast8_t>(Tm()->tm_sec); }
		uint_fast8_t Minute()Ι{ return static_cast<uint_fast8_t>(Tm()->tm_min); }
		uint_fast8_t Hour()Ι{ return static_cast<uint_fast8_t>(Tm()->tm_hour); }
		uint_fast8_t Day()Ι{ return static_cast<uint_fast8_t>(Tm()->tm_mday); }
		uint_fast8_t Month()Ι{ return static_cast<uint_fast8_t>(Tm()->tm_mon+1); }
		string MonthAbbrev()Ι;
		uint_fast16_t Year()Ι{ return static_cast<uint_fast16_t>(Tm()->tm_year+1900); }

		TimePoint Date()Ι{ return Clock::from_time_t( TimeT()/(60*60*24)*(60*60*24) ); }
#ifdef _MSC_VER
		Ω Today()ι->TimePoint{ return Clock::from_time_t( _time64(nullptr)/(60*60*24)*(60*60*24) ); }
#else
		Ω Today()ι->TimePoint{ return Clock::from_time_t(time(nullptr) / (60 * 60 * 24) * (60 * 60 * 24)); }
#endif
		α DateDisplay()Ι->string;
		α DateDisplay4()Ι->string;
		α TimeDisplay()Ι->string;
		α LocalTimeDisplay( bool seconds=false, bool milliseconds=false )Ι->string;
		α LocalDateDisplay()Ι->string;
		α LocalDisplay( bool seconds, bool milli )Ι->string;
		α ToIsoString()Ι->string;
		Ω ToIsoString(const tm& timeStruct)ι->string;
		Ω ParseMonth( sv month )ε->uint8;
		time_t TimeT()Ι;
		TimePoint GetTimePoint()Ι{return _time_point;}
		operator TimePoint()Ι{ return _time_point; }
		up<std::tm> LocalTm()Ι;
		sp<std::tm> Tm()const  ι;

		Γ friend std::ostream& operator<<( std::ostream &os, const Jde::DateTime& obj )ι;
		Ω ToDate( TimePoint time )ι->TimePoint;
	private:
		TimePoint _time_point;
		mutable up<time_t> _pTime{nullptr};
		mutable sp<std::tm> _pTm{nullptr};
	};

	Ξ ToIsoString( TimePoint time )ι->string{ return DateTime(time).ToIsoString(); }
	Ξ ToIsoString( fs::file_time_type time )ι->string{ return DateTime(time).ToIsoString(); }
	Ξ LocalTimeDisplay( TimePoint time, bool seconds=false, bool milliseconds=false )ι->string{ return DateTime{time}.LocalTimeDisplay(seconds, milliseconds); }
	Ξ DateDisplay( TimePoint time )ι->string{ return DateTime{time}.DateDisplay(); }
	Ξ DateDisplay( DayIndex day )ι->string{ return DateTime{Chrono::FromDays(day)}.DateDisplay(); }
}
#define var const auto
namespace Jde::Timezone
{
	Γ Duration GetGmtOffset( sv name, TimePoint utc, SRCE )ε;
	Γ Duration TryGetGmtOffset( sv name, TimePoint utc, SRCE )ι;
	Γ Duration EasternTimezoneDifference( TimePoint time, SRCE )ε;
	Ξ EasternTimeNow(SRCE)ε->TimePoint{ var now=Clock::now(); return now+EasternTimezoneDifference(now, sl); };
}

namespace Jde::TimeSpan
{
	constexpr static size_t NanosPerMicro{ 1000 };
	constexpr static size_t MicrosPerMilli{ 1000 };
	constexpr static size_t MilliPerSecond{ 1000 };
	constexpr static size_t SecondsPerMinute{ 60 };
	constexpr static size_t MinutesPerHour{ 60 };
	constexpr static size_t HoursPerDay{ 24 };
	constexpr static size_t SecondsPerHour{ SecondsPerMinute*MinutesPerHour };
	constexpr static size_t SecondsPerDay{ SecondsPerHour*HoursPerDay };
	constexpr static size_t NanosPerSecond{ NanosPerMicro*MicrosPerMilli*MilliPerSecond };
	constexpr static size_t NanosPerMinute{ NanosPerSecond*SecondsPerMinute };
}

namespace Jde::Chrono
{
	Ξ ToTimePoint( uint16 year, uint8 month, uint8 day, uint8 hour=0, uint8 minute=0, uint8 second=0, Duration nanoFraction=Duration{0} )ι->TimePoint{ return DateTime(year,month, day, hour, minute,second, nanoFraction).GetTimePoint(); }
	Ξ EndOfMonth( TimePoint time )ι->TimePoint{ DateTime date{time}; return DateTime(date.Year()+(date.Month()==12 ? 1 : 0), date.Month()%12+1, 1).GetTimePoint()-1s; }
	Ξ to_timepoint( sv iso )ι->TimePoint{ return DateTime{iso}.GetTimePoint(); }
	Ξ EndOfDay( TimePoint time)ι->TimePoint{ DateTime date{time}; return DateTime(date.Year(), date.Month(), date.Day(), 23, 59, 59).GetTimePoint(); }
	Ξ BeginningOfDay( TimePoint time)ι->TimePoint{ DateTime date{time}; return DateTime(date.Year(), date.Month(), date.Day(), 0, 0, 0).GetTimePoint(); }
	Ξ BeginningOfMonth( TimePoint time={} )ι->TimePoint{ DateTime date{time==TimePoint{} ? Clock::now() : time }; return DateTime{date.Year(), date.Month(), 1}.GetTimePoint(); }
	//Ξ Display( time_t t, bool seconds=false, bool milli=false )ι->string{ return DateTime{t}.LocalDisplay(seconds, milli); }
	Ξ Display( TP t, bool seconds=false, bool milli=false )ι->string{ return DateTime{t}.LocalDisplay(seconds, milli); }
	Ξ TimeDisplay( time_t t ) ι->string{ return DateTime{t}.TimeDisplay(); }
	ẗ ToClock( typename V::time_point from )ι->typename K::time_point{ return K::now()-milliseconds{ duration_cast<milliseconds>(V::time_point::clock::now()-from) }; }
}
namespace Jde{
	template<class T>
	α Chrono::ToString( T d )ι->string{
		ostringstream os;
		os << 'P';
		#define output(period,suffix) if( d>=period{1} || d<=period{-1} ){ os << duration_cast<period>(d).count() << suffix; d%=period{1}; }
		if constexpr( _msvc ){
			constexpr auto year = hours(24 * 365);
			if( d >= year || d <= -year ){
				os << duration_cast<hours>(d).count()/year.count() << "Y";
				d %= year;
			}
			constexpr auto month = hours(24 * 30);
			if( d >= month || d <= -month ){
				os << duration_cast<hours>(d).count() / month.count() << "M";
				d %= month;
			}
			constexpr auto days = hours(24);
			if( d >= days || d <= -days ){
				os << duration_cast<hours>(d).count() / days.count() << "M";
				d %= days;
			}
		}
		else{
			output( years, "Y" )
			output( months, "M" )
			output( days, "D" )
		}
		if( d!=Duration::zero() ){
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
#undef var
#undef output
#endif