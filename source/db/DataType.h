#pragma once
#include <variant>
DISABLE_WARNINGS
#include <nlohmann/json.hpp>
ENABLE_WARNINGS
#include "../math/Decimal.h"


namespace Jde::DB
{
	struct Syntax;
	using DBClock=std::chrono::system_clock;
	using DBTimePoint=DBClock::time_point;
	enum class EObject: uint8 {Null,String,StringView,StringPtr,Bool,Int,Int64,Uint,Decimal2,Double,DoubleOptional,DateOptional };
	using object=std::variant<std::nullptr_t,string,sv,sp<string>,bool,int,_int,uint,Decimal2,double,std::optional<double>,std::optional<DBTimePoint>>;
	α ToString( const object& parameter, SRCE )noexcept(false)->string;

	enum class EType:uint8{None,Int16,Int,UInt,SmallFloat,Float,Bit,Decimal,Int8,Long,ULong,Guid,Binary,VarBinary,VarTChar,VarWChar,Numeric,DateTime,Cursor,TChar,VarChar,RefCursor,SmallDateTime,WChar,NText,Text,Image,Blob,Money,Char,TimeSpan,Uri,UInt8,UInt16 };

	α ToType( sv typeName )noexcept->EType;
	α ToString( EType type, const Syntax& syntax )noexcept->string;

	α ToObject( EType type, const nlohmann::json& j, sv memberName, SRCE )noexcept(false)->object;

}
