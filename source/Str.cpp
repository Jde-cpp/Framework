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
/*	α ci_traits::compare( const char* s1, const char* s2, size_t n )ι->int
	{
	}
	α ci_traits::find( const char* s, size_t n, char a )ι->const char*
	{
		while( n-- > 0 && toupper(*s) != toupper(a) )
			++s;
		//DEBUG_IF( n==0 );
		//DEBUG_IF( strncmp(s, "ion number:", n)==0 );
		return n==string::npos ? nullptr : s;
	}

	uint CIString::find( sv sub, uint i )Ι
   {
		uint j=0;
		for( ; i+sub.size()<=size() && j<sub.size(); ++i )
			for( j=0; i<size() && j<sub.size() && traits_type::eq( (*this)[i], sub[j] ); ++i, ++j );
      return j==sub.size() ? i-sub.size()-1 : npos;
	}


	α Str::Split( sv s, sv delim )ι->vector<sv>
	{
		vector<sv> tokens;
		uint i=0;
		for( uint next = s.find(delim); next!=string::npos; i=next+delim.size(), next = s.find(delim, i) )
			tokens.push_back( s.substr(i, next-i) );
		if( i<s.size() )
			tokens.push_back( s.substr(i) );
		return tokens;
	}

	α Str::Split( sv text, const CIString& delim )ι->vector<sv>
	{
		vector<sv> tokens;
		uint i=0;
		const CIString s{text};
		for( uint next = s.find(delim); next!=string::npos; i=next+delim.size(), next = s.find(delim, i) )
			tokens.push_back( text.substr(i, next-i) );
		if( i<s.size() )
			tokens.push_back( text.substr(i) );
		return tokens;
	}
*/
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
	//	s.erase( std::find_if(s.rbegin(), s.rend(), [](int ch) {return !std::isspace(ch);}).base(), s.end() ); 
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
		//vector<string> results;
		//for( uint iStart{0}, iEnd; iStart<x.size(); iStart=iEnd )
		//{
		//	auto ch=x[iStart];
		//	for( ; ch>0 && (::isspace(ch) || ch=='.'); ch= ++iStart<x.size() ? x[iStart] : '\0' );//msvc asserts if ch<0
		//	iEnd=iStart;
		//	for( ; iEnd<x.size() && (ch<0 || (!::isspace(ch) && ch!='.')); ch=++iEnd<x.size() ? x[iEnd] : '\0' );
		//	if( var length = iEnd>iStart ? iEnd-iStart : 0; length )
		//		results.push_back( PorterStemmer(x.substr(iStart, length)) );
		//}
		//return results;	
	}
}
