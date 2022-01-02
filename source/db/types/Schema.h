#pragma once
#include "Table.h"

namespace Jde::DB
{
	struct Schema
	{
		α FindTableSuffix( str suffix, SRCE )const noexcept(false)->const Table&;
		α TryFindTableSuffix( str suffix )const noexcept->sp<const Table>;
		α FindDefTable( const Table& t1, const Table& t2 )const noexcept->sp<const Table>;

		flat_map<string, Column> Types;
		flat_map<string, sp<const Table>> Tables;

		Ω FromJson( sv jsonName )noexcept->string;
		Ω ToJson( sv jsonName )noexcept->string;
		Ω ToSingular( sv plural )noexcept->string;
		Ω ToPlural( sv singular )noexcept->string;
	};
#define var const auto
	Ξ Schema::FromJson( sv jsonName )noexcept->string
	{
		ostringstream sqlName;
		for( var ch : jsonName )
		{
			if( std::isupper(ch) )
				sqlName << "_" << (char)std::tolower( ch );
			else
				sqlName << ch;
		}
		return sqlName.str();
	}
	Ξ Schema::ToJson( sv jsonName )noexcept->string
	{
		ostringstream j;
		bool upper = false;
		for( var ch : jsonName )
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
	Ξ Schema::ToSingular( sv plural )noexcept->string
	{
		string result = plural.ends_with('s') ? string{plural.substr( 0, plural.size()-1 )} : string{plural};

		return result;
	}
	Ξ Schema::ToPlural( sv singular )noexcept->string
	{
		string result{ singular };
		return result.ends_with('s') ? result : result+"s";
	}
	Ξ Schema::TryFindTableSuffix( str suffix )const noexcept->sp<const Table>
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
	Ξ Schema::FindTableSuffix( str suffix, SL sl )const noexcept(false)->const Table&
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
}