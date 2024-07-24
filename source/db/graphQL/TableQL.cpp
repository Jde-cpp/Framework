#include <jde/db/graphQL/TableQL.h>
#include <jde/db/graphQL/FilterQL.h>
#include "../types/Schema.h"

#define var const auto
namespace Jde::DB{

	α TableQL::DBName()Ι->string{
		return Schema::ToPlural( DB::Schema::FromJson(JsonName) );
	}
	α TableQL::Filter()Ε->FilterQL{
		FilterQL filters;
		var filterArgs = Args.find( "filter" );
		var j = filterArgs==Args.end() ? Args : *filterArgs;
		for( var& [jsonColumnName,value] : j.items() ){
			vector<FilterValueQL> columnFilters;
			if( value.is_string() || value.is_number() || value.is_null() ) //(id: 42) or (name: "charlie") or (deleted: null)
				columnFilters.emplace_back( EQLOperator::Equal, value );
			else if( value.is_object() ){ //filter: {age: {gt: 18, lt: 60}}
				for( var& [op,opValue] : value.items() )
					columnFilters.emplace_back( Str::ToEnum<EQLOperator>(EQLOperatorStrings, op).value_or(EQLOperator::Equal), opValue );
			}
			else if( value.is_array() ) //(id: [1,2,3]) or (name: ["charlie","bob"])
				columnFilters.emplace_back( EQLOperator::In, value );
			else
				THROW("Invalid filter value type '{}'.", value.type_name() );
			filters.ColumnFilters.emplace( jsonColumnName, columnFilters );
		}
		return filters;
	}

	α MutationQL::TableSuffix()Ι->string{
		if( _tableSuffix.empty() )
			_tableSuffix = DB::Schema::ToPlural<string>( DB::Schema::FromJson<string,string>(JsonName) );
		return _tableSuffix;
	}

	α MutationQL::InputParam( sv name )Ε->json{
		var input = Input();
		var p = input.find( name ); THROW_IF( p==input.end(), "Could not find '{}' argument. {}", name, input.dump() );
		return *p;
	}
	α MutationQL::Input(SL sl)Ε->json{
		var pInput = Args.find("input"); THROW_IFSL( pInput==Args.end(), "Could not find input argument. '{}'", Args.dump() );
		return *pInput;
	}

	α ColumnQL::QLType( const DB::Column& column, SL sl )ε->string{
		string qlTypeName = "ID";
		if( !column.IsId ){
			switch( column.Type ){
			case EType::Bit:
				qlTypeName = "Boolean";
				break;
			case EType::Int16: case EType::Int: case EType::Int8: case EType::Long:
				qlTypeName = "Int";
				break;
			case EType::UInt16: case EType::UInt: case EType::ULong:
				qlTypeName = "UInt";
				break;
			case EType::SmallFloat: case EType::Float: case EType::Decimal: case EType::Numeric: case EType::Money:
				qlTypeName = "Float";
				break;
			case EType::None: case EType::Binary: case EType::VarBinary: case EType::Guid: case EType::Cursor: case EType::RefCursor: case EType::Image: case EType::Blob: case EType::TimeSpan:
				throw Exception{ sl, ELogLevel::Debug, "EType {} is not implemented.", (uint)column.Type };
			case EType::VarTChar: case EType::VarWChar: case EType::VarChar: case EType::NText: case EType::Text: case EType::Uri:
				qlTypeName = "String";
				break;
			case EType::TChar: case EType::WChar: case EType::UInt8: case EType::Char:
				qlTypeName = "Char";
			case EType::DateTime: case EType::SmallDateTime:
				qlTypeName = "DateTime";
				break;
			}
		}
		return qlTypeName;
	}
}