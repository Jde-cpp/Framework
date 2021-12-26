#pragma once
#include "DataType.h"
#include <jde/Str.h>

namespace Jde::DB
{
	struct IRow
	{
		β operator[]( uint value )const noexcept(false)->object=0;
		β GetBit( uint position, SRCE )const noexcept(false)->bool=0;
		β GetString( uint position, SRCE )const noexcept(false)->string=0;
		β GetCIString( uint position, SRCE )const noexcept(false)->CIString=0;
		β GetInt( uint position, SRCE )const->int64_t=0;
		β GetInt32( uint position, SRCE )const->int32_t=0;
		β GetIntOpt( uint position, SRCE )const->std::optional<_int> = 0;
		β GetDouble( uint position, SRCE )const->double=0;
		β GetFloat( uint position, SRCE )const->float{ return static_cast<float>( GetDouble(position, sl) ); }
		β GetDoubleOpt( uint position, SRCE )const->std::optional<double> = 0;
		β GetTimePoint( uint position, SRCE )const->DBTimePoint=0;
		β GetTimePointOpt( uint position, SRCE )const->std::optional<DBTimePoint> = 0;
		β GetUInt( uint position, SRCE )const->uint=0;
		β GetUInt32( uint position, SRCE )const->uint32_t{ return static_cast<uint32_t>(GetUInt(position, sl)); }
		β GetUInt16( uint position, SRCE )const->uint16_t{ return static_cast<uint16_t>(GetUInt(position, sl)); }
		β GetUIntOpt( uint position, SRCE )const->std::optional<uint> = 0;
		ⓣ Get( uint position, SRCE )const noexcept(false)->T;

		friend α operator>>( const IRow& row, string& str )noexcept(false)->const IRow&{ str=row.GetString(row._index++); return row; }
		friend α operator>>( const IRow& row, uint8_t& value )noexcept(false)->const IRow&{ value=static_cast<uint8_t>(row.GetUInt(row._index++)); return row; }
		friend α operator>>( const IRow& row, uint64_t& value )noexcept(false)->const IRow&{ value=row.GetUInt(row._index++); return row; }
		friend α operator>>( const IRow& row, uint32_t& value )noexcept(false)->const IRow&{ value=static_cast<uint32_t>(row.GetUInt(row._index++)); return row; }
		friend α operator>>( const IRow& row, optional<uint64_t>& value )noexcept(false)->const IRow&{ value=row.GetUIntOpt(row._index++); return row; }

		friend α operator>>( const IRow& row, long long& value )noexcept(false)->const IRow&{ value=row.GetInt(row._index++); return row; }
		friend α operator>>( const IRow& row, long& value )noexcept(false)->const IRow&{ value=static_cast<int32_t>(row.GetInt(row._index++)); return row; }
		friend α operator>>( const IRow& row, optional<long>& value )noexcept(false)->const IRow&{ auto value2=row.GetIntOpt(row._index++); if( value2.has_value()) value = static_cast<long>(value2.value()); else value=std::nullopt; return row; }
		friend α operator>>( const IRow& row, optional<long long>& value )noexcept(false)->const IRow&{ value=row.GetIntOpt(row._index++); return row; }

		friend α operator>>( const IRow& row, double& value )noexcept(false)->const IRow&{ value=row.GetDouble(row._index++); return row; }
		friend α operator>>( const IRow& row, optional<double>& value )noexcept(false)->const IRow&{ value=row.GetDoubleOpt(row._index++); return row; }

		friend α operator>>( const IRow& row, float& value )noexcept(false)->const IRow&{ value=static_cast<float>(row.GetDouble(row._index++)); return row; }
		friend α operator>>( const IRow& row, optional<float>& value )noexcept(false)->const IRow&{ auto dValue = row.GetDoubleOpt(row._index++); if( dValue.has_value() ) value=static_cast<float>(dValue.value()); else value=std::nullopt; return row; }

		friend α operator>>( const IRow& row, DBTimePoint& value)->const IRow&{ value=row.GetTimePoint(row._index++); return row; }
		friend α operator>>( const IRow& row, optional<DBTimePoint>& value)->const IRow&{ value=row.GetTimePointOpt(row._index++); return row; }

	protected:
		mutable uint _index{0};
	};

	template<> Ξ IRow::Get<string>( uint position, SL sl )const->string{ return GetString(position, sl); }
	template<> Ξ IRow::Get<uint>( uint position, SL sl )const->uint{ return GetUInt(position, sl); }
	template<> Ξ IRow::Get<unsigned int>( uint position, SL sl )const->unsigned int{ return (unsigned int)GetUInt(position, sl); }
	template<> Ξ IRow::Get<optional<uint32>>( uint position, SL sl )const->optional<uint32>{ const auto p = GetUIntOpt(position, sl); return p ? optional<uint32>{static_cast<uint32>(*p)} : optional<uint32>{}; }
	template<> Ξ IRow::Get<uint8>( uint position, SL sl )const->uint8{ return (uint8)GetUInt16(position, sl); }
	template<> Ξ IRow::Get<CIString>( uint position, SL sl )const->CIString{ return GetCIString(position, sl); }
}