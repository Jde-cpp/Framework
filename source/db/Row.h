#pragma once
#include "DataType.h"
#include "../StringUtilities.h"
#include "../math/Decimal.h"

namespace Jde::DB
{
	struct IRow
	{
		//virtual DataValue operator[]( uint value )const=0;
		virtual DataValue operator[]( uint value )const=0;
		virtual bool GetBit( uint position )const=0;
		virtual std::string GetString( uint position )const=0;
		virtual CIString GetCIString( uint position )const=0;
		virtual int64_t GetInt( uint position )const=0;
		virtual int32_t GetInt32( uint position )const=0;
		virtual std::optional<_int> GetIntOpt( uint position )const=0;
		virtual double GetDouble( uint position )const=0;
		virtual float GetFloat( uint position )const{ return static_cast<float>( GetDouble(position) ); }
		virtual std::optional<double> GetDoubleOpt( uint position )const=0;
		virtual DBDateTime GetDateTime( uint position )const=0;
		virtual std::optional<DBDateTime> GetDateTimeOpt( uint position )const=0;
		virtual uint GetUInt( uint position )const=0;
		virtual uint32_t GetUInt32(uint position )const{ return static_cast<uint32_t>(GetUInt(position)); }
		virtual uint16_t GetUInt16(uint position )const{ return static_cast<uint16_t>(GetUInt(position)); }
		virtual std::optional<uint> GetUIntOpt( uint position )const=0;
		template<typename T>
		T Get( uint position )const;

		friend const IRow& operator>>(const IRow& row, string& str){ str=row.GetString(row._index++); return row; }
		friend const IRow& operator>>(const IRow& row, uint8_t& value){ value=static_cast<uint8_t>(row.GetUInt(row._index++)); return row; }
		friend const IRow& operator>>(const IRow& row, uint64_t& value){ value=row.GetUInt(row._index++); return row; }
		friend const IRow& operator>>(const IRow& row, uint32_t& value){ value=static_cast<uint32_t>(row.GetUInt(row._index++)); return row; }
		friend const IRow& operator>>(const IRow& row, optional<uint64_t>& value){ value=row.GetUIntOpt(row._index++); return row; }

		friend const IRow& operator>>(const IRow& row, long long& value){ value=row.GetInt(row._index++); return row; }
		friend const IRow& operator>>(const IRow& row, long& value){ value=static_cast<int32_t>(row.GetInt(row._index++)); return row; }
		friend const IRow& operator>>(const IRow& row, optional<long>& value){ auto value2=row.GetIntOpt(row._index++); if( value2.has_value()) value = static_cast<long>(value2.value()); else value=std::nullopt; return row; }
		friend const IRow& operator>>(const IRow& row, optional<long long>& value){ value=row.GetIntOpt(row._index++); return row; }

		friend const IRow& operator>>(const IRow& row, double& value){ value=row.GetDouble(row._index++); return row; }
		friend const IRow& operator>>(const IRow& row, optional<double>& value){ value=row.GetDoubleOpt(row._index++); return row; }

		friend const IRow& operator>>(const IRow& row, float& value){ value=static_cast<float>(row.GetDouble(row._index++)); return row; }
		friend const IRow& operator>>(const IRow& row, optional<float>& value){ auto dValue = row.GetDoubleOpt(row._index++); if( dValue.has_value() ) value=static_cast<float>(dValue.value()); else value=std::nullopt; return row; }

		friend const IRow& operator>>( const IRow& row, Decimal2& value ){ value = Decimal2( row.GetDouble(row._index++) ); return row; }
		friend const IRow& operator>>( const IRow& row, optional<Decimal2>& value ){ const auto doubleValue = row.GetDoubleOpt(row._index++); value = doubleValue.has_value() ? Decimal2(value.value()) : optional<Decimal2>(); return row; }

		friend const IRow& operator>>(const IRow& row, DBDateTime& value){ value=row.GetDateTime(row._index++); return row; }
		friend const IRow& operator>>(const IRow& row, optional<DBDateTime>& value){ value=row.GetDateTimeOpt(row._index++); return row; }

	protected:
		mutable uint _index{0};
	};

	template<> inline string IRow::Get<string>( uint position )const{ return GetString(position); }
	template<> inline uint IRow::Get<uint>( uint position )const{ return GetUInt(position); }
	template<> inline unsigned int IRow::Get<unsigned int>( uint position )const{ return (unsigned int)GetUInt(position); }
	template<> inline optional<uint32> IRow::Get<optional<uint32>>( uint position )const{ const auto p = GetUIntOpt(position); return p ? optional<uint32>{*p} : optional<uint32>{}; }
	template<> inline uint8 IRow::Get<uint8>( uint position )const{ return (uint8)GetUInt16(position); }
	template<> inline CIString IRow::Get<CIString>( uint position )const{ return GetCIString(position); }

}