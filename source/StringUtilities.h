#pragma once
#include <string>
#include <sstream>
#include <list>
#include <functional>
#include <cctype>
#include "Exception.h"
#include "./log/Logging.h"

using namespace std;
namespace Jde
{
	namespace StringUtilities
	{
		template<typename T>
		std::basic_string<T> RTrim(std::basic_string<T> &s);

		template<typename T>
		std::vector<std::basic_string<T>> Split( const std::basic_string<T> &s, T delim=T{','} );

		std::vector<std::string> Split( const std::string& s, const string& delim );
		JDE_NATIVE_VISIBILITY std::vector<std::string> Split( std::string_view s, char delim=',' );

		template<typename T>
		std::string AddSeparators( T collection, string_view separator, bool quote=false );

		template<typename T>
		std::string AddCommas( T value, bool quote=false ){ return AddSeparators( value, ",", quote ); }

		std::wstring PorterStemmer(const std::wstring &s);

		JDE_NATIVE_VISIBILITY string Replace( string_view source, string_view find, string_view replace )noexcept;
		JDE_NATIVE_VISIBILITY string Replace( string_view source, char find, char replace )noexcept;
		JDE_NATIVE_VISIBILITY string ToLower( const string& source )noexcept;
		[[nodiscard]]inline string ToLower( const string_view source )noexcept{ return ToLower( string{source}); }
		JDE_NATIVE_VISIBILITY string ToUpper( const string& source )noexcept;

		template<typename T>
		static optional<T> TryTo( string_view value );

		template<typename T>
		static float TryToFloat( const std::basic_string<T>& s );

		template<typename Enum, typename Collection>
		string_view FromEnum( const Collection& s, Enum value )noexcept;
		template<typename Enum, typename Collection>
		Enum ToEnum( const Collection& s, string_view text, Enum dflt )noexcept;

		[[nodiscard]]inline bool EndsWith( const std::string_view value, const std::string_view& ending ){ return ending.size() > value.size() ? false : std::equal( ending.rbegin(), ending.rend(), value.rbegin() ); }
		[[nodiscard]]inline bool StartsWith( const std::string_view value, const std::string_view& starting ){ return starting.size() > value.size() ? false : std::equal( starting.begin(), starting.end(), value.begin() ); }
		[[nodiscard]]inline bool StartsWithInsensitive( const std::string_view value, const std::string_view& starting );

		inline void LTrim( std::string &s ){ s.erase( s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {return !std::isspace(ch); }) ); }
		inline void RTrim( std::string &s ){ s.erase( std::find_if(s.rbegin(), s.rend(), [](int ch) {return !std::isspace(ch);}).base(), s.end() ); }
		inline void Trim( std::string &s ){ LTrim(s); RTrim(s); }
	};

	struct ci_char_traits : public char_traits<char>
	{
		JDE_NATIVE_VISIBILITY static bool eq(char c1, char c2) { return toupper(c1) == toupper(c2); }
		JDE_NATIVE_VISIBILITY static bool ne(char c1, char c2) { return toupper(c1) != toupper(c2); }
		JDE_NATIVE_VISIBILITY static bool lt(char c1, char c2) { return toupper(c1) <  toupper(c2); }
		JDE_NATIVE_VISIBILITY static int compare( const char* s1, const char* s2, size_t n );
		JDE_NATIVE_VISIBILITY static const char* find( const char* s, int n, char a );
	};
	struct CIString : public std::basic_string<char, ci_char_traits>
	{
		typedef std::basic_string<char, ci_char_traits> base;
		CIString( string_view sv )noexcept:base{sv.data(), sv.size()}{}
		CIString( const string& s )noexcept:base{s.data(), s.size()}{}
		inline bool operator ==( string_view s )const noexcept{ return size()==s.size() && base::compare( 0, s.size(), s.data(), s.size() )==0; }
		inline bool operator !=( string_view s )const noexcept{ return size() == s.size() && base::compare(0, s.size(), s.data(), s.size())!=0; }
		inline bool operator !=( const string& s )const noexcept{ return *this!=string_view{s}; }
		operator string()const noexcept{ return string{data(), size()}; }

	};

	template<typename T>
	std::basic_string<T> StringUtilities::RTrim(std::basic_string<T> &s)
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
	std::string StringUtilities::AddSeparators( T collection, string_view separator, bool quote )
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
	std::vector<std::basic_string<T>> StringUtilities::Split( const std::basic_string<T> &s, T delim/*=T{','}*/ )
	{
		vector<basic_string<T>> tokens;
		basic_string<T> token;
		std::basic_istringstream<T> tokenStream(s);
		while( std::getline(tokenStream, token, delim) )
			tokens.push_back(token);

		return tokens;
	}

	template<typename T>
	float StringUtilities::TryToFloat( const std::basic_string<T>& token )
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

	template<typename T>
	static optional<T> StringUtilities::TryTo( string_view value )
	{
		optional<T> v;
		try
		{
			v = static_cast<T>( std::stoull(string(value)) );
		}
		catch( const std::invalid_argument& e )
		{
			ERR( "Can't convert:  {}.  to {}.  {}"sv, value, Jde::GetTypeName<T>(), e.what() );
		}
		return v;
	}
	inline bool StringUtilities::StartsWithInsensitive( const std::string_view value, const std::string_view& starting )
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
	Enum StringUtilities::ToEnum( const Collection& stringValues, string_view text, Enum dflt )noexcept
	{
		auto value = static_cast<Enum>( std::distance(std::find(std::begin(stringValues), std::end(stringValues), text), std::begin(stringValues)) );
		return static_cast<uint>(value)<stringValues.size() ? value : dflt;
	}
	template<typename Enum, typename Collection>
	string_view StringUtilities::FromEnum( const Collection& stringValues, Enum value )noexcept
	{
		return static_cast<uint>(value)<stringValues.size() ? stringValues[value] : string_view{};
	}
}
