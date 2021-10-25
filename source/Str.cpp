#include <jde/Str.h>

#include <algorithm>
#include <functional>
#include <locale>

namespace Jde
{

	α ci_char_traits::compare( const char* s1, const char* s2, size_t n )noexcept->int
	{
		while( n-- != 0 )
		{
			if( toupper(*s1) < toupper(*s2) ) return -1;
			if( toupper(*s1) > toupper(*s2) ) return 1;
			++s1; ++s2;
		}
		return 0;
	}
	α ci_char_traits::find( const char* s, size_t n, char a )noexcept->const char*
	{
		while( n-- > 0 && toupper(*s) != toupper(a) )
			++s;
		return s;
	}

	uint CIString::find( sv sub, uint i )const noexcept
   {
		uint j=0;
		for( ; i+sub.size()<=size() && j<sub.size(); ++i )
			for( j=0; i<size() && j<sub.size() && traits_type::eq( (*this)[i], sub[j] ); ++i, ++j );
      return j==sub.size() ? i-sub.size()-1 : npos;
	}


	α Str::Split( sv s, sv delim )->vector<sv>
	{
		vector<sv> tokens;
		uint i=0;
		for( uint next = s.find(delim); next!=string::npos; i=next+delim.size(), next = s.find(delim, i) )
			tokens.push_back( s.substr(i, next-i) );
		if( i<s.size() )
			tokens.push_back( s.substr(i) );
		return tokens;
	}
	α Str::Split( sv text, const CIString& delim )->vector<sv>
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
	α Str::Split( sv s, char delim, uint estCnt )->vector<sv>
	{
		vector<sv> results;
		if( estCnt )
			results.reserve( estCnt );
		for( uint fieldStart=0, iField=0, fieldEnd;fieldStart<s.size();++iField, fieldStart = fieldEnd+1 )
		{
			fieldEnd = std::min( s.find_first_of(delim, fieldStart), s.size() );
			results.push_back( sv{s.data()+fieldStart, fieldEnd-fieldStart} );
		};
		return results;
	}

	α Str::Replace( sv source, sv find, sv replace )noexcept->string
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
	α Str::Replace( sv source, char find, char replace )noexcept->string
	{
		string result{ source };
		for( char* pFrom=(char*)source.data(), *pTo=(char*)result.data(); pFrom<source.data()+source.size(); ++pFrom, ++pTo )
			*pTo = *pFrom==find ? replace : *pFrom;

		return result;
	}
#pragma warning( disable: 4244 )
	string Transform( sv source, function<int(int)> fnctn )noexcept
	{
		string result{ source };
		std::transform( result.begin(), result.end(), result.begin(), fnctn );
		return result;
	}
	α Str::ToLower( sv source )noexcept->string
	{
		return Transform( source, ::tolower );
	}

	string Str::ToUpper( sv source )noexcept
	{
		return Transform( source, ::toupper );
	}

	sv Str::NextWord( sv x )noexcept
	{
		uint iStart = 0;
		for( ;iStart<x.size() && x[iStart]>0 && ::isspace(x[iStart]); ++iStart );//msvc asserts if ch<0
		uint iEnd=iStart;
		for( ;iEnd<x.size() && (x[iEnd]<0 || !::isspace(x[iEnd])); ++iEnd );
		return x.substr( iStart, iEnd>iStart ? iEnd-iStart : 0 );
	}
}
