#pragma once
#include <numeric>
#include "../JdeAssert.h"

namespace Jde::Math
{
	JDE_NATIVE_VISIBILITY size_t URound( double value );
	//inline _int Round( double value ){ return static_cast<_iint>( llround(value) ); }
	template<typename T>
	struct StatResult
	{
/*		constexpr StatResult()=default;
		StatResult( T average, T variance, T min, T max ):
			Average{average}, Variance{variance}, Min{min}, Max{max}
		{}
		StatResult& operator=(const Jde::Math::StatResult&)=default;*/
		T Average{0.0};
		T Variance{0.0};
		T Min{0.0};
		T Max{0.0};
	};

#define var const auto
	template<typename TCollection>
	StatResult<typename TCollection::value_type> Statistics( const TCollection& values, bool calcVariance=true )
	{
		typedef typename TCollection::value_type T;
		var size = values.size();
		ASSERT( size>0 );
		T sum{};
		T min{ std::numeric_limits<T>::max() };
		T max{ std::numeric_limits<T>::min() };
		T average{};
		T variance{};
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
				variance = static_cast<T>( v2 );
			}
		}
		return StatResult<T>{ average, variance, min, max };
	}
#undef var
}