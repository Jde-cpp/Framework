#include <codecvt>
#include <boost/algorithm/string/trim.hpp>
#include <jde/Str.h>
#include <algorithm>
#include <functional>
#ifdef _MSC_VER
	#include "../../Windows/source/WindowsUtilities.h"
#endif

#define var const auto

namespace Jde
{
	α Str::Replace( sv source, char find, char replace )ι->string
	{
		string result{ source };
		for( char* pFrom=(char*)source.data(), *pTo=(char*)result.data(); pFrom<source.data()+source.size(); ++pFrom, ++pTo )
			*pTo = *pFrom==find ? replace : *pFrom;

		return result;
	}

	α Str::LTrim_( string& s )->void
	{
		boost::trim_left( s );
#ifdef _MSC_VER
		auto w = Windows::ToWString( s );
		boost::trim_left( w );
#else
		std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
		std::u32string utf32 = cvt.from_bytes( s );
		::boost::algorithm::trim_left_if(utf32, boost::is_any_of(U"\x2000\x2001\x2002\x2003\x2004\x2005\x2006\x2007\x2009\x200A\x2028\x2029\x202f\x205f\x3000"));
		s = cvt.to_bytes( utf32 );
#endif
	}
	//https://stackoverflow.com/questions/59589243/utf8-strings-boost-trim
	α Str::RTrim_( string& s )->void
	{
		boost::trim_right( s );
		std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
		std::u32string utf32 = cvt.from_bytes( s );
		::boost::algorithm::trim_right_if(utf32, boost::is_any_of(U"\x2000\x2001\x2002\x2003\x2004\x2005\x2006\x2007\x2009\x200A\x2028\x2029\x202f\x205f\x3000"));
		s = cvt.to_bytes( utf32 );
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

	α Str::StemmedWords( sv x )ι->vector<string>
	{
		return get<0>( Internal::WordsLocation<string,true>(string{x}) );
	}
}