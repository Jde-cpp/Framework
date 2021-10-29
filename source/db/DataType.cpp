#include "DataType.h"
#include "../DateTime.h"
#include <jde/Str.h>
#include "Syntax.h"

#define var const auto

namespace Jde
{
	using std::get;
	using nlohmann::json;
	string DB::to_string( const DataValue& parameter )
	{
		ostringstream os;
		constexpr sv nullString = "null"sv;
		switch( (EDataValue)parameter.index() )
		{
		case EDataValue::Null:
			os << nullString;
		break;
		case EDataValue::String:
			os << get<string>(parameter);
		break;
		case EDataValue::StringView:
			os << get<sv>(parameter);
		break;
		case EDataValue::StringPtr:
		{
			var& pValue = get<sp<string>>(parameter);
			if( pValue )
			{
				var str = *pValue;
				os << str;
			}
			else
				os << nullString;
		}
		break;
		case EDataValue::Bool:
			os << get<bool>(parameter);
		break;
		case EDataValue::Int:
			os << get<int>(parameter);
		break;
		case EDataValue::Int64:
			os << get<_int>(parameter);
		break;
		case EDataValue::Uint:
			os << get<uint>(parameter);
		break;
		case EDataValue::Decimal2:
			os << get<Decimal2>( parameter );
		break;
		case EDataValue::Double:
			os << get<double>( parameter );
		break;
		case EDataValue::DoubleOptional:
		{
			var& value = get<optional<double>>(parameter);
			if( value.has_value() )
				os << value.value();
			else
				os << nullString;
		}
		break;
		case EDataValue::DateOptional:
		{
			var& value = get<optional<DBDateTime>>( parameter );
			string value2 = value.has_value() ? Jde::to_string(value.value()) : string(nullString);
			os << value2;
		}
		break;
		default:
			THROW( "{} not implemented", parameter.index() );
		}
		return os.str();
	}

	DB::DataType DB::ToDataType( sv t )noexcept
	{
		CIString typeName{ t };
		DataType type{ DataType::None };
		if( typeName=="dateTime" )
			type=DataType::DateTime;
		else if( typeName=="smallDateTime" )
			type=DataType::SmallDateTime;
		else if( typeName=="float" )
			type=DataType::Float;
		else if( typeName=="real" )
			type=DataType::SmallFloat;
		else if( typeName=="bool" )
			type=DataType::Bit;
		else if( typeName=="int" )
			type = DataType::Int;
		else if( typeName=="uint32" )
			type = DataType::UInt;
		else if( typeName=="uint64" )
			type = DataType::ULong;
		else if( typeName=="Long" )
			type = DataType::Long;
		else if( typeName=="nvarchar" )
			type = DataType::VarWChar;
		else if(typeName=="nchar")
			type = DataType::WChar;
		else if( typeName=="smallint" )
			type = DataType::Int16;
		else if( typeName=="uint16" )
			type = DataType::UInt16;
		else if( typeName=="int8" )
			type = DataType::Int8;
		else if( typeName=="uint8" )
			type = DataType::UInt8;
		else if( typeName=="guid" )
			type = DataType::Guid;
		else if(typeName=="varbinary")
			type = DataType::VarBinary;
		else if( typeName=="varchar" )
			type = DataType::VarChar;
		else if( typeName=="ntext" )
			type = DataType::NText;
		else if( typeName=="text" )
			type=DataType::Text;
		else if( typeName=="char" )
			type=DataType::Char;
		else if( typeName=="image" )
			type=DataType::Image;
		else if( typeName=="bit" )
			type=DataType::Bit;
		else if( typeName=="binary" )
			type=DataType::Binary;
		else if( typeName=="decimal" )
			type=DataType::Decimal;
		else if( typeName=="numeric" )
			type=DataType::Numeric;
		else if( typeName=="money" )
			type=DataType::Money;
		else
			TRACE( "Unknown datatype({})."sv, typeName.c_str() );
		return type;
	}

	string DB::ToString( DataType type, const Syntax& syntax )noexcept
	{
		string typeName;
		if( syntax.HasUnsigned() && type == DataType::UInt ) typeName = "int unsigned";
		else if( type == DataType::Int || type == DataType::UInt ) typeName = "int";
		else if( syntax.HasUnsigned() && type == DataType::ULong ) typeName = "bigint(20) unsigned";
		else if( type == DataType::Long || type == DataType::ULong ) typeName="bigint";
		else if( type == DataType::DateTime ) typeName = "datetime";
		else if( type == DataType::SmallDateTime )typeName = "smalldatetime";
		else if( type == DataType::Float ) typeName = "float";
		else if( type == DataType::SmallFloat )typeName = "real";
		else if( type == DataType::VarWChar ) typeName = "nvarchar";
		else if( type == DataType::WChar ) typeName = "nchar";
		else if( syntax.HasUnsigned() && type == DataType::UInt16 ) typeName="smallint";
		else if( type == DataType::Int16 || type == DataType::UInt16 ) typeName="smallint";
		else if( syntax.HasUnsigned() && type == DataType::UInt8 ) typeName =  "tinyint unsigned";
		else if( type == DataType::Int8 || type == DataType::UInt8 ) typeName = "tinyint";
		else if( type == DataType::Guid ) typeName = "uniqueidentifier";
		else if( type == DataType::VarBinary ) typeName = "varbinary";
		else if( type == DataType::VarChar ) typeName = "varchar";
		else if( type == DataType::VarTChar ) typeName = "varchar";
		else if( type == DataType::NText ) typeName = "ntext";
		else if( type == DataType::Text ) typeName = "text";
		else if( type == DataType::Char ) typeName = "char";
		else if( type == DataType::Image ) typeName = "image";
		else if( type == DataType::Bit ) typeName="bit";
		else if( type == DataType::Binary ) typeName = "binary";
		else if( type == DataType::Decimal ) typeName = "decimal";
		else if( type == DataType::Numeric ) typeName = "numeric";
		else if( type == DataType::Money ) typeName = "money";
		else ERR( "Unknown datatype({})."sv, type );
		return typeName;
	}

	DB::DataValue DB::ToDataValue( DataType type, const json& j, sv memberName )
	{
		DB::DataValue value{ nullptr };
		if( !j.is_null() )
		{
			switch( type )
			{
			case DataType::Bit:
				THROW_IF( !j.is_boolean(), "{} could not conver to boolean {}", memberName, j.dump() );
				value = DB::DataValue{ j.get<bool>() };
				break;
			case DataType::Int16:
			case DataType::Int:
			case DataType::Int8:
			case DataType::Long:
				THROW_IF( !j.is_number(), "{} could not conver to int {}", memberName, j.dump() );
				value = DB::DataValue{ j.get<_int>() };
				break;
			case DataType::UInt16:
			case DataType::UInt:
			case DataType::ULong:
				THROW_IF( !j.is_number(), "{} could not conver to uint {}", memberName, j.dump() );
				value = DB::DataValue{ j.get<uint>() };
				break;
			case DataType::SmallFloat:
			case DataType::Float:
			case DataType::Decimal:
			case DataType::Numeric:
			case DataType::Money:
				THROW_IF( !j.is_number(), "{} could not conver to numeric {}", memberName, j.dump() );
				value = DB::DataValue{ j.get<double>() };
				break;
			case DataType::None:
			case DataType::Binary:
			case DataType::VarBinary:
			case DataType::Guid:
			case DataType::Cursor:
			case DataType::RefCursor:
			case DataType::Image:
			case DataType::Blob:
			case DataType::TimeSpan:
				THROW( "DataType {} is not implemented.", type );
			case DataType::VarTChar:
			case DataType::VarWChar:
			case DataType::VarChar:
			case DataType::NText:
			case DataType::Text:
			case DataType::Uri:
				THROW_IF( !j.is_string(), "{} could not conver to string", memberName );
				value = DB::DataValue{ j.get<string>() };
				break;
			case DataType::TChar:
			case DataType::WChar:
			case DataType::UInt8:
			case DataType::Char:
				THROW( "char DataType {} is not implemented." );
			case DataType::DateTime:
			case DataType::SmallDateTime:
				THROW_IF( !j.is_string(), "{} could not conver to string for datetime", memberName );
				const string time{ j.get<string>() };
				const Jde::DateTime dateTime{ time };
				const TimePoint t = dateTime.GetTimePoint();
				value = DB::DataValue{ t };
				break;
			}
		}
		return value;
	}
}