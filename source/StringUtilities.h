#pragma once
#include <string>
#include <sstream>
#include <list>
#include <functional>
#include <cctype>
#include <charconv>
//#include "Exception.h"
#include "./log/Logging.h"


namespace Jde
{
	//using std::sv;
	//using std::string;
	using std::basic_string;
	namespace StringUtilities
	{
		template<typename T>
		basic_string<T> RTrim(basic_string<T> &s);

		template<typename T>
		std::vector<basic_string<T>> Split( const basic_string<T> &s, T delim=T{','} );

		JDE_NATIVE_VISIBILITY std::vector<sv> Split( sv s, sv delim );
		JDE_NATIVE_VISIBILITY std::vector<sv> Split( sv s, char delim=',', uint estCnt=0 );

		template<typename T>
		std::string AddSeparators( T collection, sv separator, bool quote=false );

		template<typename T>
		std::string AddCommas( T value, bool quote=false ){ return AddSeparators( value, ",", quote ); }

		JDE_NATIVE_VISIBILITY sv NextWord( sv x )noexcept;

		std::wstring PorterStemmer(const std::wstring &s);

		JDE_NATIVE_VISIBILITY string Replace( sv source, sv find, sv replace )noexcept;
		JDE_NATIVE_VISIBILITY string Replace( sv source, char find, char replace )noexcept;
		[[nodiscard]] JDE_NATIVE_VISIBILITY string ToLower( sv source )noexcept;
		//inline string ToLower( sv source )noexcept{ return ToLower( string{source}); }
		JDE_NATIVE_VISIBILITY string ToUpper( sv source )noexcept;

		template<typename T>
		static optional<T> TryTo( sv value )noexcept;
		template<typename T>
		static T To( sv value )noexcept(false);

		template<typename T>
		static float TryToFloat( const basic_string<T>& s )noexcept;
		optional<double> TryToDouble( const std::string& s )noexcept;

		template<typename Enum, typename Collection>
		sv FromEnum( const Collection& s, Enum value )noexcept;
		template<typename Enum, typename Collection>
		Enum ToEnum( const Collection& s, sv text, Enum dflt )noexcept;

		[[nodiscard]]inline bool EndsWith( sv value, sv ending ){ return ending.size() > value.size() ? false : std::equal( ending.rbegin(), ending.rend(), value.rbegin() ); }
		[[nodiscard]]inline bool StartsWith( sv value, sv starting ){ return starting.size() > value.size() ? false : std::equal( starting.begin(), starting.end(), value.begin() ); }
		[[nodiscard]]inline bool StartsWithInsensitive( sv value, sv starting );

		inline void LTrim( std::string &s ){ s.erase( s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {return !std::isspace(ch); }) ); }
		inline void RTrim( std::string &s ){ s.erase( std::find_if(s.rbegin(), s.rend(), [](int ch) {return !std::isspace(ch);}).base(), s.end() ); }
		inline void Trim( std::string &s ){ LTrim(s); RTrim(s); }
		inline std::string Trim( const std::string &s ){ auto y{s}; LTrim(y); RTrim(y); return y; }
	};

	struct ci_char_traits : public std::char_traits<char>
	{
		JDE_NATIVE_VISIBILITY static bool eq(char c1, char c2) { return toupper(c1) == toupper(c2); }
		JDE_NATIVE_VISIBILITY static bool ne(char c1, char c2) { return toupper(c1) != toupper(c2); }
		JDE_NATIVE_VISIBILITY static bool lt(char c1, char c2) { return toupper(c1) <  toupper(c2); }
		JDE_NATIVE_VISIBILITY static int compare( const char* s1, const char* s2, size_t n );
		JDE_NATIVE_VISIBILITY static const char* find( const char* s, int n, char a );
	};
	struct CIString : public basic_string<char, ci_char_traits>
	{
		typedef basic_string<char, ci_char_traits> base;
		CIString()noexcept{};
		CIString( sv sv )noexcept:base{sv.data(), sv.size()}{}
		CIString( str s )noexcept:base{s.data(), s.size()}{}
		inline bool operator ==( sv s )const noexcept{ return size()==s.size() && base::compare( 0, s.size(), s.data(), s.size() )==0; }
		inline bool operator ==( const char* psz )const noexcept{ return size()==strlen(psz) && base::compare( 0, size(), psz, size() )==0; }
		inline bool operator !=( sv s )const noexcept{ return size() == s.size() && base::compare(0, s.size(), s.data(), s.size())!=0; }
		inline bool operator !=( str s )const noexcept{ return *this!=sv{s}; }
		operator string()const noexcept{ return string{data(), size()}; }
	};
#define var const auto
	template<typename T>
	basic_string<T> StringUtilities::RTrim(basic_string<T> &s)
	{
#ifdef _MSC_VER
		for( uint i=s.size()-1; i>=0; --i )
		{
			if( isspace(s[i]) )
				s.erase( *s.rbegin() );
			else
				break;
		}
#else
		s.erase( std::find_if(s.rbegin(), s.rend(), [](int ch){return !std::isspace(ch);}).base(), s.end() );
#endif
		return s;
	}

	template<typename T>
	std::string StringUtilities::AddSeparators( T collection, sv separator, bool quote )
	{
		ostringstream os;
		auto first = true;
		for( const auto& item : collection )
		{
			if( first )
				first = false;
			else
				os << separator;
			if( quote )
				os << "'" << item << "'";
			else
				os << item;
		}
		return os.str();
	}

	template<typename T>
	std::vector<basic_string<T>> StringUtilities::Split( const basic_string<T> &s, T delim/*=T{','}*/ )
	{
		vector<basic_string<T>> tokens;
		basic_string<T> token;
		std::basic_istringstream<T> tokenStream(s);
		while( std::getline(tokenStream, token, delim) )
			tokens.push_back(token);

		return tokens;
	}

	template<typename T>
	float StringUtilities::TryToFloat( const basic_string<T>& token )noexcept
	{
		try
		{
			return std::stof( token );
		}
		catch(std::invalid_argument e)
		{
			GetDefaultLogger()->trace( "Can't convert:  {}.  to float.  {}", token, e.what() );
			return std::nanf("");
		}
	}
	inline optional<double> StringUtilities::TryToDouble( const std::string& s )noexcept
	{
		optional<double> v;
		try
		{
			v = std::stod( s );
		}
		catch(std::invalid_argument e)
		{
			TRACE( "Can't convert:  {}.  to float.  {}"sv, s, e.what() );
		}
		return v;
	}
	template<typename T>
	static optional<T> StringUtilities::TryTo( sv value )noexcept
	{
		optional<T> v;
		Try( [&v, value]{v=To<T>( value );} );
		return v;
	}
	template<typename T>
	static T StringUtilities::To( sv value )
	{
		T v;
		var e=std::from_chars( value.data(), value.data()+value.size(), v );
		THROW_IF( e.ec!=std::errc(), "Can't convert:  '{}'.  to '{}'.  ec='{}'"sv, value, Jde::GetTypeName<T>(), (uint)e.ec );
		return v;
	}
	inline bool StringUtilities::StartsWithInsensitive( sv value, sv starting )
	{
		bool equal = starting.size() <= value.size();
		if( equal )
		{
			for( uint i=0; i<starting.size(); ++i )
			{
				equal = ::toupper(starting[i])==::toupper(value[i]);
				if( !equal )
					break;
			}
		}
		return equal;
	}

	template<typename Enum, typename Collection>
	Enum StringUtilities::ToEnum( const Collection& stringValues, sv text, Enum dflt )noexcept
	{
		auto value = static_cast<Enum>( std::distance(std::find(std::begin(stringValues), std::end(stringValues), text), std::begin(stringValues)) );
		return static_cast<uint>(value)<stringValues.size() ? value : dflt;
	}
	template<typename Enum, typename Collection>
	sv StringUtilities::FromEnum( const Collection& stringValues, Enum value )noexcept
	{
		return static_cast<uint>(value)<stringValues.size() ? stringValues[value] : sv{};
	}
#undef var
}
