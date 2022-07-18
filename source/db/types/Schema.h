#pragma once
#include "Table.h"

#define TMPL template<class T=string>
#define BSV Str::bsv<typename T::traits_type>
#define RESULT std::basic_string<char,typename T::traits_type>
#define YRESULT std::basic_string<char,typename Y::traits_type>
namespace Jde::DB
{
	struct Schema
	{
		α FindTableSuffix( sv suffix, SRCE )const noexcept(false)->const Table&;
		α TryFindTableSuffix( sv suffix )const noexcept->sp<const Table>;
		α FindDefTable( const Table& t1, const Table& t2 )const noexcept->sp<const Table>;

		template<class X=string,class Y=string> Ω FromJson( Str::bsv<typename X::traits_type> jsonName )noexcept->YRESULT;
		template<class X=string,class Y=string> Ω ToJson(   Str::bsv<typename X::traits_type> schemaName )noexcept->YRESULT;
		Ω ToSingular( sv plural )noexcept->sv;
		TMPL Ω ToPlural( BSV singular )noexcept->RESULT;

		flat_map<string, Column> Types;
		flat_map<SchemaName, sp<const Table>> Tables;
	};
#define var const auto
	template<class X,class Y> α Schema::FromJson( Str::bsv<typename X::traits_type> jsonName )noexcept->YRESULT
	{
		YRESULT sqlName; sqlName.reserve( jsonName.size() );
		for( var ch : jsonName )
		{
			if( std::isupper(ch) )
			{
				sqlName+="_";
				sqlName +=(char)std::tolower( ch );
			}
			else
				sqlName+=ch;
		}
		return sqlName;
	}
	template<class X,class Y> α Schema::ToJson( Str::bsv<typename X::traits_type> schemaName )noexcept->YRESULT
	{
		ostringstream j;
		bool upper = false;
		for( var ch : schemaName )
		{
			if( ch=='_' )
				upper = true;
			else if( upper )
			{
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
	Ξ Schema::ToSingular( sv plural )noexcept->sv
	{
		return plural.ends_with('s') ? plural.substr( 0, plural.size()-1 ) : plural;
	}
	Ŧ Schema::ToPlural( BSV singular )noexcept->RESULT
	{
		return singular.ends_with( 's' ) ? RESULT{ singular } : RESULT{ singular }+"s";
	}
	Ξ Schema::TryFindTableSuffix( sv suffix )const noexcept->sp<const Table>
	{
		sp<const Table> y;
		for( var& [name,pTable] : Tables )
		{
			if( name.ends_with(suffix) && name.size()>suffix.size()+2 && name[name.size()-suffix.size()-1]=='_' && name.substr(0,name.size()-suffix.size()-1).find_first_of('_')==string::npos )
			{
				y = pTable;
				break;
			}
		}
		return y;
	}
	Ξ Schema::FindTableSuffix( sv suffix, SL sl )const noexcept(false)->const Table&
	{
		var y = TryFindTableSuffix( suffix );
		if( !y ) throw Exception{ sl, ELogLevel::Debug, "Could not find table '{}' in schema", suffix };//mysql can't use THROW_IF
		return *y;
	}
	Ξ Schema::FindDefTable( const Table& t1, const Table& t2 )const noexcept->sp<const Table>
	{
		sp<const Table> result;
		var singularT1 = t1.FKName();
		var singularT2 = t2.FKName();
		for( var& [name,pTable] : Tables )
		{
			var childId = pTable->ChildId(); if( childId.empty() ) continue;
			var parentId = pTable->ParentId(); if( parentId.empty() ) continue;

			if( (singularT1==childId || singularT1==parentId) && (singularT2==childId || singularT2==parentId) )
			{
				result = pTable;
				break;
			}
		}
		return result;
	}
#undef var
#undef TMPL
#undef BSV
#undef RESULT
#undef YRESULT
}