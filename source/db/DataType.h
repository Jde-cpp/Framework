#pragma once
#include <variant>
#include "../math/Decimal.h"

namespace Jde::DB
{
	typedef std::chrono::system_clock DBClock;
	typedef DBClock::time_point DBDateTime;
	enum class EDataValue: uint8 {Null,String,StringView,StringPtr,Bool,Int,Int64,Uint,Decimal2,Double,DoubleOptional,DateOptional };
	typedef std::variant<std::nullptr_t,string,string_view,sp<string>,bool,int,_int,uint,Decimal2,double,std::optional<double>,std::optional<DBDateTime>> DataValue;
	string to_string( const DataValue& parameter );
	enum DataType
	{
		//[XmlEnum("none")]
		None,
		//[XmlEnum("int16")]
		Int16,
		//[XmlEnum("int")]
		Int,
		//[XmlEnum( "uint" )]
		UInt,
		//[XmlEnum( "float4" )]
		SmallFloat,
		//[XmlEnum("float")]
		Float,
		//[XmlEnum("bit")]
		Bit,
		//[XmlEnum("decimal")]
		Decimal,
		//[XmlEnum("int8")]
		Int8,
		//[XmlEnum("long")]
		Long,
		//[XmlEnum( "ulong" )]
		ULong,
		//[XmlEnum("guid")]
		Guid,
		//[XmlEnum("binary")]
		Binary,
		//[XmlEnum("vbinary")]
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
		Uri
	};
}
