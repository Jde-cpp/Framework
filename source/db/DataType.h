#pragma once
#include <variant>
#pragma warning(push)
#pragma warning( disable : 4715)
#include <nlohmann/json.hpp>
#pragma warning(pop)
#include "../math/Decimal.h"
#include <nlohmann/json.hpp>

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
		//[XmlEnum("vtchar")]
		VarTChar,
		//[XmlEnum("vwchar")]
		VarWChar,
		//[XmlEnum("numeric")]
		Numeric,
		//[XmlEnum("date_time")]
		DateTime,
		//[XmlEnum("cursor")]
		Cursor,
		//[XmlEnum("tchar")]
		TChar,
		//[XmlEnum("vchar")]
		VarChar,
		//[XmlEnum("ref_cursor")]
		RefCursor,
		//[XmlEnum("small_date_time")]
		SmallDateTime,
		//[XmlEnum("wchar")]
		WChar,
		//[XmlEnum("ntext")]
		NText,
		//[XmlEnum("text")]
		Text,
		//[XmlEnum("image")]
		Image,
		//[XmlEnum("blob")]
		Blob,
		//[XmlEnum("money")]
		Money,
		//[XmlEnum("char")]
		Char,
		//[XmlEnum( "time_span" )]
		TimeSpan,
		//[XmlEnum( "uri" )]
		Uri,
		UInt8,
		UInt16
	};
	DataType ToDataType( sv typeName )noexcept;
	string ToString( DataType type, const Syntax& syntax )noexcept;

	DataValue ToDataValue( DataType type, const nlohmann::json& j, sv memberName )noexcept(false);

}
