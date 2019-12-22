#pragma once
#include <ctime>
#include <chrono>
#include <iostream>
#include <memory>
#include "./Exports.h"
#include "TypeDefs.h"

namespace Jde
{
	using namespace std::literals::chrono_literals;
	typedef uint16 DayIndex;
	namespace Chrono
	{
		JDE_NATIVE_VISIBILITY TimePoint Epoch()noexcept;
		inline uint MillisecondsSinceEpoch(const TimePoint& time)noexcept{ return duration_cast<std::chrono::milliseconds>( time-Epoch() ).count(); }
		inline DayIndex DaysSinceEpoch(const TimePoint& time)noexcept{ return duration_cast<std::chrono::hours>( time-Epoch()).count()/24; }
		inline TimePoint FromDays( DayIndex days )noexcept{ return Epoch()+days*24h; }
		inline TimePoint Date( const TimePoint& time )noexcept{ return Clock::from_time_t( Clock::to_time_t(time)/(60*60*24)*(60*60*24) ); }
		inline Duration Time( const TimePoint& time )noexcept{ return time-Date(time); }

		namespace TimeSpan
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
	}
/*	struct JDE_NATIVE_VISIBILITY TimeSpan //TODO Refactor
	{
		constexpr TimeSpan( _int nanos ):_ticks( nanos ){}
		constexpr TimeSpan( Clock::duration& duration ):_ticks( duration ){}
		TimeSpan operator-()const noexcept{return TimeSpan(8);}
		Clock::duration Ticks()const noexcept{ return _ticks; }
		Clock::duration Parse( string_view stringValue );//https://en.wikipedia.org/wiki/ISO_8601 - "P0003-06-04T12:30:05
		//constexpr string_view RegEx = "/^(R\d*\/)?P(?:\d+(?:\.\d+)?Y)?(?:\d+(?:\.\d+)?M)?(?:\d+(?:\.\d+)?W)?(?:\d+(?:\.\d+)?D)?(?:T(?:\d+(?:\.\d+)?H)?(?:\d+(?:\.\d+)?M)?(?:\d+(?:\.\d+)?S)?)?$/,";
		//constexpr string_view RegEx2 = "^P(?!$)(\d+Y)?(\d+M)?(\d+W)?(\d+D)?(T(?=\d)(\d+H)?(\d+M)?(\d+S)?)?$";

#pragma warning (disable:4244)
		constexpr static TimeSpan FromDays( double value ){ return TimeSpan( value*static_cast<double>(HoursPerDay*MinutesPerHour*SecondsPerMinute*NanosPerSecond) ); }
#pragma warning (default:4244)
	};*/
	enum class DayOfWeek : uint8
	{
		Sunday=0,
		Monday=1,
		Tuesday=2,
		Wednesday=3,
		Thursday=4,
		Friday=5,
		Saturday=6
	};
	inline const TimePoint& min( const TimePoint& t1, const TimePoint& t2 ){ return t2<t1 ? t2 : t1;}
	inline const TimePoint& max( const TimePoint& t1, const TimePoint& t2 ){ return t2>t1 ? t2 : t1;}
	struct JDE_NATIVE_VISIBILITY DateTime
	{
		DateTime()noexcept;
		DateTime( const DateTime& other )noexcept;
		DateTime( time_t time )noexcept;
		DateTime( uint16 year, uint8 month, uint8 day, uint8 hour=0, uint8 minute=0, uint8 second=0, Duration nanoFraction=Duration{0} )noexcept;
		DateTime( string_view iso )noexcept(false);
		//DateTime( uint16 year, uint8 month, uint8 day, uint8 hour=0, uint8 minute=0, uint8 second=0, uint8 milliseconds )noexcept;
		DateTime( const TimePoint& tp )noexcept;
		//std::chrono::time_point<Clock, std::chrono::duration<long int, std::ratio<1, 1000000000> > >
		static DateTime BeginingOfWeek();
		DateTime& operator=(const DateTime& other)noexcept;
		bool operator==(const DateTime& other)const noexcept{return _time_point==other._time_point; }
		bool operator<(const DateTime& other)const noexcept{return _time_point<other._time_point;}
		//DateTime operator+( const TimeSpan& timeSpan )const;
		DateTime operator+( const Duration& timeSpan )const;
		//DateTime& operator+=( const TimeSpan& timeSpan );
		DateTime& operator+=( const Duration& timeSpan );
		//DateTime operator-( const TimeSpan& timeSpan )const;
		//DateTime& operator-=( const TimeSpan& timeSpan );

		static DayOfWeek DayOfWk( const TimePoint& time )noexcept{ return DateTime{time}.DayOfWk();}
		DayOfWeek DayOfWk()const noexcept{ return (DayOfWeek)(Tm()->tm_wday);}
		uint32 Nanos()const noexcept;
		uint_fast8_t Second()const noexcept{ return static_cast<uint_fast8_t>(Tm()->tm_sec); }
		uint_fast8_t Minute()const noexcept{ return static_cast<uint_fast8_t>(Tm()->tm_min); }
		uint_fast8_t Hour()const noexcept{ return static_cast<uint_fast8_t>(Tm()->tm_hour); }
		uint_fast8_t Day()const noexcept{ return static_cast<uint_fast8_t>(Tm()->tm_mday); }
		uint_fast8_t Month()const noexcept{ return static_cast<uint_fast8_t>(Tm()->tm_mon+1); }
		string MonthAbbrev()const noexcept;
		uint_fast16_t Year()const noexcept{ return static_cast<uint_fast16_t>(Tm()->tm_year+1900); }

		//time_t LocalDate()const noexcept{ return TimeT()/(60*60*24); }
		TimePoint Date()const noexcept{ return Clock::from_time_t( TimeT()/(60*60*24)*(60*60*24) ); }
		static TimePoint Today()noexcept{ return Clock::from_time_t( time(nullptr)/(60*60*24)*(60*60*24) ); }
		std::string DateDisplay()const noexcept;
		std::string DateDisplay4()const noexcept;
		std::string TimeDisplay()const noexcept;
		std::string LocalTimeDisplay()const noexcept;
		std::string LocalDateDisplay()const noexcept;
		std::string LocalDisplay()const noexcept;
		std::string ToIsoString()const noexcept;
		static std::string ToIsoString(const tm& timeStruct)noexcept;
		static uint ParseMonth( string_view month )noexcept(false);
		time_t TimeT()const noexcept;
		const TimePoint& GetTimePoint()const noexcept{return _time_point;}
		operator TimePoint()const noexcept{ return _time_point; }
		std::unique_ptr<std::tm> LocalTm()const noexcept;
		std::shared_ptr<std::tm> Tm()const  noexcept;

		JDE_NATIVE_VISIBILITY friend std::ostream& operator<<( std::ostream &os, const Jde::DateTime& obj )noexcept;
		static TimePoint ToDate( const TimePoint& time )noexcept;
	private:
//		void swap( const DateTime& other );
		TimePoint _time_point;
		mutable std::unique_ptr<time_t> _pTime{nullptr};
		mutable std::shared_ptr<std::tm> _pTm{nullptr};

		//std::tm	_tm;
	};

	inline string ToIsoString( const TimePoint& time )noexcept{ return DateTime(time).ToIsoString(); }//TODOrefactor
	inline string to_string( const TimePoint& time ){ return DateTime(time).ToIsoString(); }

	namespace TimeZone
	{
		JDE_NATIVE_VISIBILITY Duration EasternTimeZoneDifference( const TimePoint& time )noexcept;
	}
	namespace Chrono
	{
		inline TimePoint ToTimePoint( uint16 year, uint8 month, uint8 day, uint8 hour=0, uint8 minute=0, uint8 second=0, Duration nanoFraction=Duration{0} )noexcept{ return DateTime(year,month, day, hour, minute,second, nanoFraction).GetTimePoint(); }
		inline TimePoint EndOfMonth( const TimePoint& time )noexcept{ DateTime date{time}; return DateTime(date.Year()+(date.Month()==12 ? 1 : 0), date.Month()%12+1, 1).GetTimePoint()-1s; }
		inline TimePoint to_timepoint( string_view iso )noexcept{ return DateTime{iso}.GetTimePoint(); }
		inline string DateDisplay(const TimePoint& time)noexcept{ return DateTime{time}.DateDisplay(); }
		inline string DateDisplay(DayIndex day)noexcept{ return DateTime{FromDays(day)}.DateDisplay(); }
		inline TimePoint EndOfDay(const TimePoint& time){ DateTime date{time}; return DateTime(date.Year(), date.Month(), date.Day(), 23, 59, 59).GetTimePoint(); }
		inline TimePoint BeginningOfDay(const TimePoint& time){ DateTime date{time}; return DateTime(date.Year(), date.Month(), date.Day(), 0, 0, 0).GetTimePoint(); }
	}
}
JDE_NATIVE_VISIBILITY std::ostream& operator<<( std::ostream &os, const std::chrono::system_clock::time_point& obj )noexcept;

