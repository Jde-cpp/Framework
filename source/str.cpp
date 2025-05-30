#include <jde/framework/str.h>
#include <algorithm>
#include <functional>
#include <boost/algorithm/hex.hpp>
#include <fmt/args.h>

#ifdef _MSC_VER
	#include <jde/framework/process/os/windows/WindowsUtilities.h>
#endif

#define let const auto

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
		let p = NextWordLocation( x );
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
    fmt::dynamic_format_arg_store<fmt::format_context> store;
		for( auto&& arg : args )
			store.push_back( move(arg) );
    return fmt::vformat(format, store);
	}
}