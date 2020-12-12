#pragma once
#include <sstream>
#include "../TypeDefs.h"

//https://stackoverflow.com/questions/21806561/concatenating-strings-and-numbers-in-variadic-template-function
namespace Jde::ToVec
{
	inline void Append( vector<str>& /*values*/ ){}

	template<typename Head, typename... Tail>
	void Append( vector<str>& values, Head&& h, Tail&&... t );

	template<typename... Tail>
	void Append( vector<str>& values, str&& h, Tail&&... t )
	{
		values.push_back( h );
		return Apend( values, std::forward<Tail>(t)... );
	}

	template<typename T>
	str ToStringT( const T& x )
	{
		ostringstream os;
		os << x;
		return os.str();
	}

	template<typename Head, typename... Tail>
	void Append( vector<str>& values, Head&& h, Tail&&... t )
	{
		values.push_back( ToStringT(std::forward<Head>(h)) );
		return Append( values, std::forward<Tail>(t)... );
	}
}
