#pragma once
#ifndef SCHEMA_H
#define SCHEMA_H
#include "Table.h"

#define BSV Str::bsv<typename T::traits_type>
#define RESULT std::basic_string<char,typename T::traits_type>
#define YRESULT std::basic_string<char,typename Y::traits_type>
namespace Jde::DB{
	struct Schema{
		α FindTable( str name, SRCE )Ε->const Table&;
		α TryFindTable( str name )Ι->sp<const Table>;
		α FindTableSuffix( sv suffix, SRCE )Ε->const Table&;// um_users find users.
		α TryFindTableSuffix( sv suffix )Ι->sp<const Table>;
		α FindDefTable( const Table& t1, const Table& t2 )Ι->sp<const Table>;//route, route_steps-> -> route_definition

		template<class X=string,class Y=string> Ω FromJson( Str::bsv<typename X::traits_type> jsonName )ι->YRESULT;
		template<class X=string,class Y=string> Ω ToJson(   Str::bsv<typename X::traits_type> schemaName )ι->YRESULT;
		Ω ToSingular( sv plural )ι->string;
		template<class T=string> Ω ToPlural( BSV singular )ι->RESULT;

		flat_map<string, Column> Types;
		flat_map<SchemaName, sp<const Table>> Tables;
	};
#define var const auto
	template<class X,class Y> α Schema::FromJson( Str::bsv<typename X::traits_type> jsonName )ι->YRESULT{
		YRESULT sqlName; sqlName.reserve( jsonName.size() );
		for( var ch : jsonName ){
			if( std::isupper(ch) ){
				sqlName+="_";
				sqlName +=(char)std::tolower( ch );
			}
			else
				sqlName+=ch;
		}
		return sqlName;
	}
	template<class X,class Y> α Schema::ToJson( Str::bsv<typename X::traits_type> schemaName )ι->YRESULT{
		ostringstream j;
		bool upper = false;
		for( var ch : schemaName ){
			if( ch=='_' )
				upper = true;
			else if( upper ){
				j << (char)std::toupper( ch );
				upper = false;
			}
			else if( j.tellp()==0 )
				j << (char)tolower( ch );
			else
				j << ch;
		}
		return j.str();
	}
	Ξ Schema::ToSingular( sv plural )ι->string{
		string y{ plural };
		if( plural.ends_with("ies") )
			y = string{plural.substr( 0, plural.size()-3 )}+"y";
		else if( plural.ends_with('s') )
			y = plural.substr( 0, plural.size()-1 );
		return y;
	}
	Ŧ Schema::ToPlural( BSV singular )ι->RESULT{
		RESULT y{ singular };
		if( singular.ends_with("y") )
			y = RESULT{ singular }.substr(0, singular.size()-1)+"ies";
		else if( !singular.ends_with('s') )
			y = RESULT{ singular }+"s";
		return y;
	}
	
	Ξ Schema::TryFindTable( str name )Ι->sp<const Table>{
		var y = Tables.find( name );
		return y==Tables.end() ? nullptr : y->second;
	}
	Ξ Schema::FindTable( str name, SL sl )Ε->const Table&{
		var y = TryFindTable( name ); if( !y ) throw Exception{ sl, ELogLevel::Debug, "Could not find table '{}' in schema", name };//mysql can't use THROW_IF
		return *y;
	}
	
	Ξ Schema::TryFindTableSuffix( sv suffix )Ι->sp<const Table>{
		sp<const Table> y;
		for( var& [name,pTable] : Tables ){
			if( name.ends_with(suffix) && name.size()>suffix.size()+2 && name[name.size()-suffix.size()-1]=='_' && name.substr(0,name.size()-suffix.size()-1).find_first_of('_')==string::npos ){
				y = pTable;
				break;
			}
		}
		return y;
	}
	Ξ Schema::FindTableSuffix( sv suffix, SL sl )Ε->const Table&{
		var y = TryFindTableSuffix( suffix );
		if( !y ) throw Exception{ sl, ELogLevel::Debug, "Could not find table '{}' in schema", suffix };//mysql can't use THROW_IF
		return *y;
	}
	// route, route_steps -> route_definition
	Ξ Schema::FindDefTable( const Table& t1, const Table& t2 )Ι->sp<const Table>{
		sp<const Table> result;
		var singularT1 = t1.FKName();
		var singularT2 = t2.FKName();
		for( var& [name,pTable] : Tables ){
			var pParentTable = pTable->ParentTable( *this );
			var pChildTable = pTable->ChildTable( *this );
			if( pParentTable && pChildTable
				&& (  (t1.Name==pParentTable->Name && t2.Name==pChildTable->Name)
						||(t2.Name==pParentTable->Name && t1.Name==pChildTable->Name) ) ){
				result = pTable;
				break;
			}
		}	
		return result;
	}
#undef var
#undef BSV
#undef RESULT
#undef YRESULT
}
#endif