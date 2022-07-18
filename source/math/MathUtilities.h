#pragma once
#include <numeric>

namespace Jde
{
	template<typename T=uint> α Round( double value )->T
	{
		return static_cast<T>( llround(value) );
	}
}
namespace Jde::Math
{
	template<typename T>
	struct StatResult
	{
		T Average{0.0};
		T Variance{0.0};
		T Min{0.0};
		T Max{0.0};
	};

#define var const auto
	Ŧ Statistics( const T& values, bool calcVariance=true )noexcept->StatResult<typename T::value_type>
	{
		typedef typename T::value_type TValue;
		var size = values.size();
		//ASSERT( size>0 );
		TValue sum{};
		TValue min{ std::numeric_limits<TValue>::max() };
		TValue max{ std::numeric_limits<TValue>::min() };
		TValue average{};
		TValue variance{};
		for( var& value : values )
		{
			sum += value;
			min = std::min( min, value );
			max = std::max( max, value );
		}
		if( size>0 )
		{
			average = sum/size;
			if( size>1 && calcVariance )
			{
				auto varianceFunction = [&average, &size]( double accumulator, const double& val )
				{
					var diff = val - average;
					return accumulator + diff*diff / (size - 1);//sample?
				};
				double v2 = std::accumulate( values.begin(), values.end(), 0.0, varianceFunction );
				variance = static_cast<TValue>( v2 );
			}
		}
		return StatResult<TValue>{ average, variance, min, max };
	}
#undef var
}