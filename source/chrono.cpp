#include <jde/framework/chrono.h>
#include "math/MathUtilities.h"

#define let const auto
namespace Jde{
	α Chrono::LocalTimeMilli( TimePoint time, SL sl )ε->string{
		try{
			auto tp = std::chrono::current_zone()->to_local( time );
			return std::format( "{0:%H:%M:}{1:%S}", tp, time.time_since_epoch() );
		}
		catch( const std::exception& e ){
			throw Exception{ sl, "Failed to convert time point to local time: {} - {}", ToIsoString(time), e.what() };
		}
	}
	α Chrono::ToDuration( string&& iso )ε->Duration{
		std::istringstream is{ move(iso) };
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
	α Chrono::ToTimePoint( string iso, SL sl )ε->TimePoint{
		TimePoint tp;
		std::istringstream is{ move(iso) };
		is >> std::chrono::parse( "%FT%TZ", tp );
		THROW_IFSL( is.fail(), "Could not parse ISO time '{}'", iso );
		return tp;
	}
	α Chrono::ToTimePoint( uint16_t y, uint8_t mnth, uint8_t dayOfMonth, uint8 h, uint8 mnt, uint8 scnd, SL sl )ε->TimePoint{
    auto ymd = year{y}/month{mnth}/day{dayOfMonth};
		THROW_IFSL( !ymd.ok(), "Invalid date: {}-{}-{}", y, mnth, dayOfMonth );
    auto tp = sys_days{ymd} + hours{h} + minutes{mnt} + seconds{scnd};
		return tp;
	}
}