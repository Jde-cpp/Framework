#pragma once
#include "Table.h"

namespace Jde::DB
{
	//struct Table;
	struct Schema
	{
		static string FromJson( sv jsonName )noexcept;
		static string ToJson( sv jsonName )noexcept;
		static string ToSingular( sv plural )noexcept;
		static string ToPlural( sv singular )noexcept;
		sp<const Table> FindTableSuffix( sv suffix )const noexcept;
		sp<const Table> FindDefTable( const Table& t1, const Table& t2 )const noexcept;
		//FindJsonType( sv jsonType )const noexcept;
		flat_map<string, Column> Types;
		flat_map<string, sp<const Table>> Tables;
	};
#define var const auto
	inline string Schema::FromJson( sv jsonName )noexcept
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
	inline string Schema::ToJson( sv jsonName )noexcept
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
	inline string Schema::ToSingular( sv plural )noexcept
	{
		string result = plural.ends_with('s') ? string{plural.substr( 0, plural.size()-1 )} : string{plural};

		return result;
	}
	inline string Schema::ToPlural( sv singular )noexcept
	{
		string result{ singular };
		return result.ends_with('s') ? result : result+"s";
	}
	inline sp<const Table> Schema::FindTableSuffix( sv suffixNoUnder )const noexcept
	{
		var suffix = string{ "_"+string{suffixNoUnder} };
		sp<const Table> result;
		for( var& [name,pTable] : Tables )
		{
			if( name.ends_with(suffix) && name.size()>suffix.size()+1 && name.substr(0,name.size()-suffix.size()).find_first_of('_')==string::npos )
			{
				result = pTable;
				break;
			}
		}
		return result;
	}
	inline sp<const Table> Schema::FindDefTable( const Table& t1, const Table& t2 )const noexcept
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
