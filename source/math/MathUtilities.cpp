#include "stdafx.h"
#include "MathUtilities.h"
#include <numeric>

namespace Jde::Math
{
	size_t URound( double value )
	{
		return static_cast<uint>( llround(value) );
	}
}