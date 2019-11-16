#include "stdafx.h"
#include "../DateTime.h"
#include "DataType.h"

#define var const auto

namespace Jde::DB
{
	using std::get;
	string to_string( const DataValue& parameter )
	{
		ostringstream os;
		constexpr string_view nullString = "null"sv;
		switch( (EDataValue)parameter.index() )
		{
		case EDataValue::Null:
			os << nullString;
		break;
		case EDataValue::String:
			os << get<string>(parameter);
		break;
		case EDataValue::StringView:
			os << get<string_view>(parameter);
		break;
		case EDataValue::StringPtr:
		{
			var& pValue = get<sp<string>>(parameter);
			os << pValue ? *pValue : nullString;
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
			os << value.has_value() ? Jde::to_string(value.value()) : nullString;
		}
		break;
		default:
			throw Exception( "{} not implemented", parameter.index() );
		}
		return os.str();
	}
}