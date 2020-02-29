//#pragma warning( push )
//#pragma warning( disable : 4996)

#include "StringUtilities.h"

#include <algorithm>
#include <functional>
#include <locale>

namespace Jde
{

	int ci_char_traits::compare( const char* s1, const char* s2, size_t n )
	{
		while( n-- != 0 )
		{
			if( toupper(*s1) < toupper(*s2) ) return -1;
			if( toupper(*s1) > toupper(*s2) ) return 1;
			++s1; ++s2;
		}
		return 0;
	}
	const char* ci_char_traits::find(const char* s, int n, char a)
	{
		while( n-- > 0 && toupper(*s) != toupper(a) )
			++s;
		return s;
	}

	std::vector<std::string> StringUtilities::Split( const string& s, const string& delim )
	{
		vector<string> tokens;
		uint i=0;
		for( uint next = s.find(delim); next!=string::npos; i=next+delim.size(), next = s.find(delim, i) )
			tokens.push_back( s.substr(i, next) );
		if( i<s.size() )
			tokens.push_back( s.substr(i) );
		return tokens;
	}
	std::vector<std::string> StringUtilities::Split( std::string_view s, char delim )
	{
		return Split<char>(string(s), delim);
	}

	string StringUtilities::Replace( string_view source, string_view find, string_view replace )noexcept
	{
		string::size_type i=0, iLast=0;
		ostringstream os;
   	while( (i = source.find(find, i))!=string::npos )
    	{
			os << source.substr( iLast, i-iLast );
			os << replace;
			i += find.length();
			iLast = i;
		}
		if( iLast<source.size() )
			os << source.substr( iLast, source.size()-iLast );

		return os.str();
	}
	string StringUtilities::Replace( string_view source, char find, char replace )noexcept
	{
		string result{ source };
		for( char* pFrom=(char*)source.data(), *pTo=(char*)result.data(); pFrom<source.data()+source.size(); ++pFrom, ++pTo )
			*pTo = *pFrom==find ? replace : *pFrom;

		return result;
	}
	string StringUtilities::ToLower( const string& source )noexcept
	{
		string result{ source };
#if _WINDOWS
		for( int i=0; i<source.size(); ++i )
			result[i] = static_cast<char>( ::tolower( result[i]) );
#else
		std::transform( result.begin(), result.end(), result.begin(), ::tolower );
#endif
		return result;
	}

	string StringUtilities::ToUpper( const string& source )noexcept
	{
		string result{ source };
		std::transform( result.begin(), result.end(), result.begin(), ::toupper );
		return result;
	}
}
