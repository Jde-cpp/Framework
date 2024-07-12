#include <jde/Str.h>
#include <algorithm>
#include <functional>
#ifdef _MSC_VER
	#include "../../Windows/source/WindowsUtilities.h"
#endif

#define var const auto

namespace Jde{
	α Str::ToString( sv format, vector<string> args )ι->string{
		using ctx = fmt::format_context;
		vector<fmt::basic_format_arg<ctx>> ctxArgs;
		for( var& a : args )
			ctxArgs.push_back( fmt::detail::make_arg<ctx>(a) );
		string y;
		try{
			y = fmt::vformat( format, fmt::basic_format_args<ctx>{ctxArgs.data(), (int)ctxArgs.size()} );
		}
		catch( const fmt::format_error& e ){
			y = Jde::format( "format error format='{}' - {}", format, e.what() );
		}
		return y;
	}

	const string empty;
	α Str::Empty()ι->str{ return empty; };

	α Str::Replace( sv source, char find, char replace )ι->string
	{
		string result{ source };
		for( char* pFrom=(char*)source.data(), *pTo=(char*)result.data(); pFrom<source.data()+source.size(); ++pFrom, ++pTo )
			*pTo = *pFrom==find ? replace : *pFrom;

		return result;
	}

#pragma warning( disable: 4244 )
	string Transform( sv source, function<int(int)> fnctn )ι
	{
		string result{ source };
		std::transform( result.begin(), result.end(), result.begin(), fnctn );
		return result;
	}
	α Str::ToLower( sv source )ι->string
	{
		return Transform( source, ::tolower );
	}

	α Str::ToUpper( sv source )ι->string
	{
		return Transform( source, ::toupper );
	}

	α Str::NextWord( sv x )ι->sv
	{
		var p = NextWordLocation( x );
		return p ? get<0>( *p ) : sv{};
	}
}