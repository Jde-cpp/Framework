#include "DataType.h"
#include "../DateTime.h"
#include <jde/Str.h>
#include "Syntax.h"

#define var const auto

namespace Jde
{
	using nlohmann::json;
	α DB::ToString( const object& parameter, SL sl )noexcept(false)->string
	{
		ostringstream os;
		constexpr sv nullString = "null"sv;
		const EObject type = (EObject)parameter.index();
		if( type==EObject::Null )
			os << nullString;
		else if( type==EObject::String )
			os << get<string>(parameter);
		else if( type==EObject::StringView )
			os << get<sv>(parameter);
		else if( type==EObject::StringPtr )
		{
			if( var& pValue = get<sp<string>>(parameter); pValue )
				os << *pValue;
			else
				os << nullString;
		}
		else if( type==EObject::Bool )
			os << get<bool>(parameter);
		else if( type==EObject::Int )
			os << get<int>(parameter);
		else if( type==EObject::Int64 )
			os << get<_int>(parameter);
		else if( type==EObject::Uint )
			os << get<uint>(parameter);
		//else if( type==EObject::Decimal2 )
		//	os << get<Decimal2>( parameter );
		else if( type==EObject::Double )
			os << get<double>( parameter );
	/*	else if( type==EObject::DoubleOptional )
		{
			var& value = get<optional<double>>(parameter);
			if( value.has_value() )
				os << value.value();
			else
				os << nullString;
		}*/
		else if( type==EObject::Time )
			os << ToIsoString( get<DBTimePoint>(parameter) );
		else
			throw Exception{ sl, Jde::ELogLevel::Debug, "{} not implemented", parameter.index() };
		return os.str();
	}

	α  DB::ToType( sv t )noexcept->DB::EType
	{
		CIString typeName{ t };
		EType type{ EType::None };
		if( typeName=="dateTime" )
			type=EType::DateTime;
		else if( typeName=="smallDateTime" )
			type=EType::SmallDateTime;
		else if( typeName=="float" )
			type=EType::Float;
		else if( typeName=="real" )
			type=EType::SmallFloat;
		else if( typeName=="bool" )
			type=EType::Bit;
		else if( typeName=="int" )
			type = EType::Int;
		else if( typeName=="uint32" )
			type = EType::UInt;
		else if( typeName=="uint64" )
			type = EType::ULong;
		else if( typeName=="Long" )
			type = EType::Long;
		else if( typeName=="nvarchar" )
			type = EType::VarWChar;
		else if(typeName=="nchar")
			type = EType::WChar;
		else if( typeName=="smallint" )
			type = EType::Int16;
		else if( typeName=="uint16" )
			type = EType::UInt16;
		else if( typeName=="int8" )
			type = EType::Int8;
		else if( typeName=="uint8" )
			type = EType::UInt8;
		else if( typeName=="guid" )
			type = EType::Guid;
		else if(typeName=="varbinary")
			type = EType::VarBinary;
		else if( typeName=="varchar" )
			type = EType::VarChar;
		else if( typeName=="ntext" )
			type = EType::NText;
		else if( typeName=="text" )
			type = EType::Text;
		else if( typeName=="char" )
			type = EType::Char;
		else if( typeName=="image" )
			type = EType::Image;
		else if( typeName=="bit" )
			type = EType::Bit;
		else if( typeName=="binary" )
			type = EType::Binary;
		else if( typeName=="decimal" )
			type = EType::Decimal;
		else if( typeName=="numeric" )
			type = EType::Numeric;
		else if( typeName=="money" )
			type = EType::Money;
		else
			TRACE( "Unknown datatype({})."sv, typeName.c_str() );
		return type;
	}

	α DB::ToString( EType type, const Syntax& syntax )noexcept->string
	{
		string typeName;
		if( syntax.HasUnsigned() && type == EType::UInt ) typeName = "int unsigned";
		else if( type == EType::Int || type == EType::UInt ) typeName = "int";
		else if( syntax.HasUnsigned() && type == EType::ULong ) typeName = "bigint(20) unsigned";
		else if( type == EType::Long || type == EType::ULong ) typeName="bigint";
		else if( type == EType::DateTime ) typeName = "datetime";
		else if( type == EType::SmallDateTime )typeName = "smalldatetime";
		else if( type == EType::Float ) typeName = "float";
		else if( type == EType::SmallFloat )typeName = "real";
		else if( type == EType::VarWChar ) typeName = "nvarchar";
		else if( type == EType::WChar ) typeName = "nchar";
		else if( syntax.HasUnsigned() && type == EType::UInt16 ) typeName="smallint";
		else if( type == EType::Int16 || type == EType::UInt16 ) typeName="smallint";
		else if( syntax.HasUnsigned() && type == EType::UInt8 ) typeName =  "tinyint unsigned";
		else if( type == EType::Int8 || type == EType::UInt8 ) typeName = "tinyint";
		else if( type == EType::Guid ) typeName = "uniqueidentifier";
		else if( type == EType::VarBinary ) typeName = "varbinary";
		else if( type == EType::VarChar ) typeName = "varchar";
		else if( type == EType::VarTChar ) typeName = "varchar";
		else if( type == EType::NText ) typeName = "ntext";
		else if( type == EType::Text ) typeName = "text";
		else if( type == EType::Char ) typeName = "char";
		else if( type == EType::Image ) typeName = "image";
		else if( type == EType::Bit ) typeName="bit";
		else if( type == EType::Binary ) typeName = "binary";
		else if( type == EType::Decimal ) typeName = "decimal";
		else if( type == EType::Numeric ) typeName = "numeric";
		else if( type == EType::Money ) typeName = "money";
		else ERR( "Unknown datatype({})."sv, (uint)type );
		return typeName;
	}

	α DB::ToObject( EType type, const json& j, sv memberName, SL sl )noexcept(false)->DB::object
	{
		object value{ nullptr };
		if( !j.is_null() )
		{
			switch( type )
			{
			case EType::Bit:
				THROW_IFX( !j.is_boolean(), Exception(sl, "{} could not conver to boolean {}", memberName, j.dump()) );
				value = object{ j.get<bool>() };
				break;
			case EType::Int16: case EType::Int: case EType::Int8: case EType::Long:
				THROW_IFX( !j.is_number(), Exception(sl, "{} could not conver to int {}", memberName, j.dump()) );
				value = object{ j.get<_int>() };
				break;
			case EType::UInt16: case EType::UInt: case EType::ULong:
				THROW_IFX( !j.is_number(), Exception(sl, "{} could not conver to uint {}", memberName, j.dump()) );
				value = object{ j.get<uint>() };
				break;
			case EType::SmallFloat: case EType::Float: case EType::Decimal: case EType::Numeric: case EType::Money:
				THROW_IFX( !j.is_number(), Exception(sl, "{} could not conver to numeric {}", memberName, j.dump()) );
				value = object{ j.get<double>() };
				break;
			case EType::None: case EType::Binary: case EType::VarBinary: case EType::Guid: case EType::Cursor: case EType::RefCursor: case EType::Image: case EType::Blob: case EType::TimeSpan:
				throw Exception{ sl, "EObject {} is not implemented.", (uint)type };
			case EType::VarTChar: case EType::VarWChar: case EType::VarChar: case EType::NText: case EType::Text: case EType::Uri:
				THROW_IFX( !j.is_string(), Exception(sl, "{} could not conver to string", memberName) );
				value = object{ j.get<string>() };
				break;
			case EType::TChar: case EType::WChar: case EType::UInt8: case EType::Char:
				throw Exception{ sl, "char EObject {} is not implemented.", (uint)type };
			case EType::DateTime: case EType::SmallDateTime:
				THROW_IFX( !j.is_string(), Exception(sl, "{} could not convert to string for datetime", memberName) );
				const string time{ j.get<string>() };
				const Jde::DateTime dateTime{ time };
				const TimePoint t = dateTime.GetTimePoint();
				value = object{ t };
				break;
			}
		}
		return value;
	}
}