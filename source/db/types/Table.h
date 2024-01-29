#pragma once
#include "../DataType.h"
#include <jde/Exports.h>
#include <jde/Str.h>

namespace Jde::DB
{
	using SchemaName=string;
	struct Syntax;
	struct Schema;
	struct Table;
	struct Γ Column
	{
		Column()=default;
		Column( sv name, uint ordinalPosition, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isIdentity, bool isId, optional<uint> numericPrecision, optional<uint> numericScale )ι;
		Column( sv name )ι;
		Column( sv name, const nlohmann::json& j, const flat_map<SchemaName,Column>& commonColumns, const flat_map<SchemaName,Table>& parents, const nlohmann::ordered_json& schema )ε;

		α Create( const Syntax& syntax )Ι->string;
		α DataTypeString( const Syntax& syntax )Ι->SchemaName;
		SchemaName Name;
		uint OrdinalPosition;
		string Default;
		bool IsNullable{ false };
		mutable bool IsFlags{ false };
		mutable bool IsEnum{ false };
		mutable const Table* TablePtr{ nullptr };
		EType Type{ EType::UInt };
		optional<uint> MaxLength;
		bool IsIdentity{ false };
		bool IsId{ false };
		optional<uint> NumericPrecision;
		optional<uint> NumericScale;
		bool Insertable{ true };
		bool Updateable{ true };
		SchemaName PKTable;
		string QLAppend;//also select this column in ql query
	};

	struct  Γ Index
	{
		Index( sv indexName, sv tableName, bool primaryKey, vector<SchemaName>* pColumns=nullptr, bool unique=true, optional<bool> clustered=optional<bool>{} )ι;//, bool clustered=false
		Index( sv indexName, sv tableName, const Index& other )ι;

		α Create( sv name, sv tableName, const Syntax& syntax )Ι->string;
		SchemaName Name;
		SchemaName TableName;
		vector<SchemaName> Columns;
		bool Clustered;
		bool Unique;
		bool PrimaryKey;
	};
	struct Γ Table
	{
		Table( sv schema, sv name )ι:Schema{schema}, Name{name}{}
		Table( sv name, const nlohmann::json& j, const flat_map<SchemaName,Table>& parents, const flat_map<SchemaName,Column>& commonColumns, const nlohmann::ordered_json& schema )ε;

		α Create( const Syntax& syntax )Ι->string;
		α InsertProcName()Ι->SchemaName;
		α InsertProcText( const Syntax& syntax )Ι->string;
		α FindColumn( sv name )Ι->const Column*;

		α IsFlags()Ι->bool{ return FlagsData.size(); }
		α IsEnum()Ι->bool{ return Data.size(); }//GraphQL attribute
		α NameWithoutType()Ι->sv;//users in um_users.
		α Prefix()Ι->sv;//um in um_users.
		α JsonTypeName()Ι->string;

		α FKName()Ι->SchemaName;
		bool IsMap()Ι{ return ChildId().size() && ParentId().size(); }
		α ChildId()Ε->SchemaName;
		α ParentId()Ε->SchemaName;
		sp<const Table> ChildTable( const DB::Schema& schema )Ε;
		sp<const Table> ParentTable( const DB::Schema& schema )Ε;

		bool HaveSequence()Ι{ return std::find_if( Columns.begin(), Columns.end(), [](const auto& c){return c.IsIdentity;} )!=Columns.end(); }
		SchemaName Schema;
		SchemaName Name;
		vector<Column> Columns;
		vector<Index> Indexes;
		vector<SchemaName> SurrogateKey;
		vector<vector<SchemaName>> NaturalKeys;
		flat_map<uint,string> FlagsData;
		vector<nlohmann::json> Data;
		bool CustomInsertProc{false};
	};
	struct ForeignKey
	{
		Ω Create( sv name, sv columnName, const DB::Table& pkTable, sv foreignTable )ε->string;

		SchemaName Name;
		SchemaName Table;
		vector<SchemaName> Columns;
		SchemaName pkTable;
	};

	struct Procedure
	{
		SchemaName Name;
	};
}