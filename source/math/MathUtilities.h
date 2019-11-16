#pragma once
#include <numeric>
#include "../JdeAssert.h"

namespace Jde::Math
{
	JDE_NATIVE_VISIBILITY size_t URound( double value );
	//inline _int Round( double value ){ return static_cast<_iint>( llround(value) ); }

	struct StatResult
	{
		constexpr StatResult()=default;
		StatResult( double average, double variance, double min, double max ):
			Average{average}, Variance{variance}, Min{min}, Max{max}
		{}
		StatResult& operator=(const Jde::Math::StatResult&)=default;
		double Average{0.0};
		double Variance{0.0};
		double Min{0.0};
		double Max{0.0};
	};

	//template<template<class> class TCollection>
	template<typename TCollection>
	StatResult Statistics( const TCollection& values, bool calcVariance=true )
	{
		const auto size = values.size();
		double sum = 0.0;
		double min = std::numeric_limits<double>::max();
		double max = std::numeric_limits<double>::min();
		double average = 0.0;
		double variance = 0.0;
		for( const auto& value : values )
		{
			sum += value;
			min = std::min( min, value );
			max = std::max( max, value );
		}
		ASSERT( size>0 );
		if( size>0 )
		{
			average = sum/size;
			if( size>1 && calcVariance )
			{
				auto varianceFunction = [&average, &size]( double accumulator, const double& val )
				{
					const auto diff = val - average;
					return accumulator + diff*diff / (size - 1);//sample?
				};
				variance = std::accumulate( values.begin(), values.end(), 0.0, varianceFunction );
			}
		}
		return StatResult( average, variance, min, max );
	}
}