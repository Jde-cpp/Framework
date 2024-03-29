﻿#include <jde/Str.h>
#include <algorithm>
#include <functional>
#ifdef _MSC_VER
	#include "../../Windows/source/WindowsUtilities.h"
#endif

#define var const auto

namespace Jde
{
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