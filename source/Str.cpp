#include <jde/Str.h>
#include <algorithm>
#include <functional>
#include <boost/algorithm/hex.hpp>
#ifdef _MSC_VER
	#include "../../Windows/source/WindowsUtilities.h"
#endif

#define var const auto

namespace Jde{
	α Str::DecodeUri( sv x )ι->string{
		auto from_hex = [](char ch) { return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10; };
		string y{}; y.reserve( x.size() );
    for (auto i = x.begin(), n = x.end(); i != n; ++i){
			char ch = *i;
      if( ch == '%' ){
        if (i[1] && i[2]){
          ch = (char)( from_hex(i[1]) << 4 | from_hex(i[2]) );
          i += 2;
        }
      }
			else if (ch == '+')
        ch = ' ';
			y+=ch;
		}
		return y;
	}

	const string _empty;
	α Str::Empty()ι->str{ return _empty; };

	α Str::NextWord( sv x )ι->sv{
		var p = NextWordLocation( x );
		return p ? get<0>( *p ) : sv{};
	}

	α Str::Replace( sv source, char find_, char replace )ι->string{
		string result{ source };
		for( char* pFrom=(char*)source.data(), *pTo=(char*)result.data(); pFrom<source.data()+source.size(); ++pFrom, ++pTo )
			*pTo = *pFrom==find_ ? replace : *pFrom;

		return result;
	}

	α Str::ToHex( byte* p, uint size )ι->string{
		string hex;
		hex.reserve( size*2 );
		boost::algorithm::hex_lower( (char*)p, (char*)p+size, std::back_inserter(hex) );
		return hex;
	}

#pragma warning( disable: 4244 )
	string Transform( sv source, function<int(int)> fnctn )ι{
		string result{ source };
		std::transform( result.begin(), result.end(), result.begin(), fnctn );
		return result;
	}
	α Str::ToLower( sv source )ι->string{ return Transform( source, ::tolower ); }
	α Str::ToUpper( sv source )ι->string{ return Transform( source, ::toupper ); }

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
			BREAK;
			y = Jde::format( "format error format='{}' - {}", format, e.what() );
		}
		return y;
	}
}