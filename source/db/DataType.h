#pragma once
#include <variant>
DISABLE_WARNINGS
#include <nlohmann/json.hpp>
ENABLE_WARNINGS
#include "../math/Decimal.h"


namespace Jde::DB
{
	struct Syntax;
	typedef std::chrono::system_clock DBClock;
	typedef DBClock::time_point DBDateTime;
	enum class EDataValue: uint8 {Null,String,StringView,StringPtr,Bool,Int,Int64,Uint,Decimal2,Double,DoubleOptional,DateOptional };
	typedef std::variant<std::nullptr_t,string,sv,sp<string>,bool,int,_int,uint,Decimal2,double,std::optional<double>,std::optional<DBDateTime>> DataValue;
	string to_string( const DataValue& parameter );

	enum DataType
	{
		None,
		Int16,
		Int,
		UInt,
		SmallFloat,
		Float,
		Bit,
		Decimal,
		Int8,
		Long,
		ULong,
		Guid,
		Binary,
		VarBinary,
		VarTChar,
		VarWChar,
		Numeric,
		DateTime,
		Cursor,
		TChar,
		VarChar,
		RefCursor,
		SmallDateTime,
		WChar,
		NText,
		Text,
		Image,
		Blob,
		Money,
		Char,
		TimeSpan,
		Uri,
		UInt8,
		UInt16
	};
	α ToDataType( sv typeName )noexcept->DataType;
	α ToString( DataType type, const Syntax& syntax )noexcept->string;

	α ToDataValue( DataType type, const nlohmann::json& j, sv memberName )noexcept(false)->DataValue;

}
