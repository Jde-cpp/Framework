#pragma once
#include <chrono>
#include <jde/TypeDefs.h>
#include <jde/Exports.h>

namespace Jde::Chrono
{
	using namespace std::chrono;
	Γ α Epoch()noexcept->TimePoint;
	Ξ MillisecondsSinceEpoch( TimePoint time)noexcept->uint{ return duration_cast<std::chrono::milliseconds>( time-Epoch() ).count(); }
	Γ α Min( TimePoint a, TimePoint b )noexcept->TimePoint;

	Ξ Date( TimePoint time )noexcept->TimePoint{ return Clock::from_time_t( Clock::to_time_t(time)/(60*60*24)*(60*60*24) ); }
	Ξ Time( TimePoint time )noexcept->Duration{ return time-Date(time); }

	Ξ FromDays( DayIndex days )noexcept->TimePoint{ return Epoch()+days*24h; }
	Ξ ToDays( TimePoint time )noexcept->DayIndex{ return duration_cast<std::chrono::hours>( time-Epoch()).count()/24; }
	Ξ ToDays( time_t time )noexcept->DayIndex{ return ToDays(Clock::from_time_t(time)); }

	Γ α ToDuration( sv iso )noexcept(false)->Duration;
	Γ α ToString( Duration d )noexcept->string;
}
namespace Jde
{
	enum class DayOfWeek : uint8{ Sunday=0, Monday=1, Tuesday=2, Wednesday=3, Thursday=4, Friday=5, Saturday=6 };

	struct Γ DateTime
	{
		DateTime()noexcept;
		DateTime( const DateTime& other )noexcept;
		DateTime( time_t time )noexcept;
		DateTime( uint16 year, uint8 month, uint8 day, uint8 hour=0, uint8 minute=0, uint8 second=0, Duration nanoFraction=Duration{0} )noexcept;
		DateTime( sv iso )noexcept(false);
		DateTime( TimePoint tp )noexcept;
		DateTime( fs::file_time_type time )noexcept;
		Ω BeginingOfWeek()->DateTime;
		DateTime& operator=(const DateTime& other)noexcept;
		bool operator==(const DateTime& other)const noexcept{return _time_point==other._time_point; }
		bool operator<(const DateTime& other)const noexcept{return _time_point<other._time_point;}
		DateTime operator+( const Duration& timeSpan )const;
		DateTime& operator+=( const Duration& timeSpan );

		Ω DayOfWk( TimePoint time )noexcept{ return DateTime{time}.DayOfWk();}
		DayOfWeek DayOfWk()const noexcept{ return (DayOfWeek)(Tm()->tm_wday);}
		uint32 Nanos()const noexcept;
		uint_fast8_t Second()const noexcept{ return static_cast<uint_fast8_t>(Tm()->tm_sec); }
		uint_fast8_t Minute()const noexcept{ return static_cast<uint_fast8_t>(Tm()->tm_min); }
		uint_fast8_t Hour()const noexcept{ return static_cast<uint_fast8_t>(Tm()->tm_hour); }
		uint_fast8_t Day()const noexcept{ return static_cast<uint_fast8_t>(Tm()->tm_mday); }
		uint_fast8_t Month()const noexcept{ return static_cast<uint_fast8_t>(Tm()->tm_mon+1); }
		string MonthAbbrev()const noexcept;
		uint_fast16_t Year()const noexcept{ return static_cast<uint_fast16_t>(Tm()->tm_year+1900); }

		TimePoint Date()const noexcept{ return Clock::from_time_t( TimeT()/(60*60*24)*(60*60*24) ); }
#ifdef _MSC_VER
		Ω Today()noexcept->TimePoint{ return Clock::from_time_t( _time64(nullptr)/(60*60*24)*(60*60*24) ); }
#else
		Ω Today()noexcept->TimePoint{ return Clock::from_time_t(time(nullptr) / (60 * 60 * 24) * (60 * 60 * 24)); }
#endif
		α DateDisplay()const noexcept->string;
		α DateDisplay4()const noexcept->string;
		α TimeDisplay()const noexcept->string;
		α LocalTimeDisplay()const noexcept->string;
		α LocalDateDisplay()const noexcept->string;
		α LocalDisplay()const noexcept->string;
		α ToIsoString()const noexcept->string;
		Ω ToIsoString(const tm& timeStruct)noexcept->string;
		Ω ParseMonth( sv month )noexcept(false)->uint8;
		time_t TimeT()const noexcept;
		TimePoint GetTimePoint()const noexcept{return _time_point;}
		operator TimePoint()const noexcept{ return _time_point; }
		up<std::tm> LocalTm()const noexcept;
		sp<std::tm> Tm()const  noexcept;

		Γ friend std::ostream& operator<<( std::ostream &os, const Jde::DateTime& obj )noexcept;
		Ω ToDate( TimePoint time )noexcept->TimePoint;
	private:
		TimePoint _time_point;
		mutable up<time_t> _pTime{nullptr};
		mutable sp<std::tm> _pTm{nullptr};
	};

	Ξ ToIsoString( TimePoint time )noexcept->string{ return DateTime(time).ToIsoString(); }
	Ξ ToIsoString( fs::file_time_type time )noexcept->string{ return DateTime(time).ToIsoString(); }
	//Ξ to_string( TimePoint time )noexcept->string{ return DateTime(time).ToIsoString(); }//TODO take out.
	Ξ DateDisplay( TimePoint time)noexcept->string{ return DateTime{time}.DateDisplay(); }
	Ξ DateDisplay( DayIndex day)noexcept->string{ return DateTime{Chrono::FromDays(day)}.DateDisplay(); }
}
#define var const auto
namespace Jde::Timezone
{
	Γ Duration GetGmtOffset( sv name, TimePoint utc )noexcept(false);
	Γ Duration TryGetGmtOffset( sv name, TimePoint utc )noexcept;
	Γ Duration EasternTimezoneDifference( TimePoint time )noexcept;
	Ξ EasternTimeNow()noexcept->TimePoint{ var now=Clock::now(); return now+EasternTimezoneDifference(now); };
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
	Ξ ToTimePoint( uint16 year, uint8 month, uint8 day, uint8 hour=0, uint8 minute=0, uint8 second=0, Duration nanoFraction=Duration{0} )noexcept->TimePoint{ return DateTime(year,month, day, hour, minute,second, nanoFraction).GetTimePoint(); }
	Ξ EndOfMonth( TimePoint time )noexcept->TimePoint{ DateTime date{time}; return DateTime(date.Year()+(date.Month()==12 ? 1 : 0), date.Month()%12+1, 1).GetTimePoint()-1s; }
	Ξ to_timepoint( sv iso )noexcept->TimePoint{ return DateTime{iso}.GetTimePoint(); }
	Ξ EndOfDay( TimePoint time)noexcept->TimePoint{ DateTime date{time}; return DateTime(date.Year(), date.Month(), date.Day(), 23, 59, 59).GetTimePoint(); }
	Ξ BeginningOfDay( TimePoint time)noexcept->TimePoint{ DateTime date{time}; return DateTime(date.Year(), date.Month(), date.Day(), 0, 0, 0).GetTimePoint(); }
	Ξ BeginningOfMonth( TimePoint time={} )noexcept->TimePoint{ DateTime date{time==TimePoint{} ? Clock::now() : time }; return DateTime{date.Year(), date.Month(), 1}.GetTimePoint(); }
	Ξ Display( time_t t )noexcept->string{ return DateTime{t}.LocalDisplay(); }
	Ξ TimeDisplay( time_t t ) noexcept->string{ return DateTime{t}.TimeDisplay(); }
	ẗ ToClock( typename V::time_point from )noexcept->typename K::time_point
	{
		return K::now()-milliseconds{ duration_cast<milliseconds>(V::time_point::clock::now()-from) };
	}
}

#undef var
