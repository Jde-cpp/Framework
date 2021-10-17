#pragma once
#include "../DataType.h"
#include <jde/Exports.h>
#include <jde/Str.h>

namespace Jde::DB
{
	using SchemaName=CIString;
	struct Syntax;
	struct Schema;
	struct Table;
	struct JDE_NATIVE_VISIBILITY Column
	{
		Column()=default;
		Column( sv name, uint ordinalPosition, sv dflt, bool isNullable, DataType type, optional<uint> maxLength, bool isIdentity, bool isId, optional<uint> numericPrecision, optional<uint> numericScale )noexcept;
		Column( sv name )noexcept;
		Column( sv name, const nlohmann::json& j, const flat_map<string,Column>& commonColumns, const flat_map<string,Table>& parents, const nlohmann::ordered_json& schema )noexcept(false);

		string Create( const Syntax& syntax )const noexcept;
		string DataTypeString(const Syntax& syntax)const noexcept;
		string Name;
		uint OrdinalPosition;
		string Default;
		bool IsNullable{false};
		mutable bool IsFlags{false};
		DataType Type{ DataType::UInt };
		optional<uint> MaxLength;
		bool IsIdentity{false};
		bool IsId{false};
		optional<uint> NumericPrecision;
		optional<uint> NumericScale;
		bool Insertable{true};
		bool Updateable{true};
		string PKTable;
	};
	struct Table;
	struct  JDE_NATIVE_VISIBILITY Index
	{
		Index( sv indexName, sv tableName, bool primaryKey, vector<CIString>* pColumns=nullptr, bool unique=true, optional<bool> clustered=optional<bool>{} )noexcept;//, bool clustered=false
		Index( sv indexName, sv tableName, const Index& other )noexcept;

		string Create( sv name, sv tableName, const Syntax& syntax )const noexcept;
		CIString Name;
		CIString TableName;
		vector<CIString> Columns;
		bool Clustered;
		bool Unique;
		bool PrimaryKey;
	};
	struct JDE_NATIVE_VISIBILITY Table
	{
		Table( sv schema, sv name )noexcept:Schema{schema}, Name{name}{}
		Table( sv name, const nlohmann::json& j, const flat_map<string,Table>& parents, const flat_map<string,Column>& commonColumns, const nlohmann::ordered_json& schema )noexcept(false);

		string Create( const Syntax& syntax )const noexcept;
		string InsertProcName()const noexcept;
		string InsertProcText( const Syntax& syntax )const noexcept;
		α FindColumn( sv name )const noexcept->const Column*;

		α IsFlags()const noexcept->bool{ return FlagsData.size(); }
		α IsEnum()const noexcept->bool{ return Data.size(); }//GraphQL attribute
		α NameWithoutType()const noexcept->string;//users in um_users.
		string Prefix()const noexcept;//um in um_users.
		string JsonTypeName()const noexcept;
		string FKName()const noexcept;
		bool IsMap()const noexcept{ return ChildId().size() && ParentId().size(); }
		string ChildId()const noexcept(false);
		string ParentId()const noexcept(false);
		sp<const Table> ChildTable( const DB::Schema& schema )const noexcept(false);
		sp<const Table> ParentTable( const DB::Schema& schema )const noexcept(false);

		bool HaveSequence()const noexcept{ return std::find_if( Columns.begin(), Columns.end(), [](const auto& c){return c.IsIdentity;} )!=Columns.end(); }
		string Schema;
		string Name;
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
		static string Create( sv name, sv columnName, const DB::Table& pkTable, sv foreignTable )noexcept(false);

		string Name;
		string Table;
		vector<string> Columns;
		string pkTable;
	};

	struct Procedure
	{
		string Name;
	};
}