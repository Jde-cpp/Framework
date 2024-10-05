#pragma once
#include "../DataType.h"
#include <jde/Exports.h>
#include <jde/Str.h>

namespace Jde::DB{
	using SchemaName=string;
	struct Syntax;
	struct Schema;
	struct Table;
	struct Γ Column{
		Column()=default;
		Column( sv name, uint ordinalPosition, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isIdentity, bool isId, optional<uint> numericPrecision, optional<uint> numericScale )ι;
		Column( sv name )ι;
		Column( sv name, const nlohmann::json& j, const flat_map<SchemaName,Column>& commonColumns, const flat_map<SchemaName,Table>& parents, const nlohmann::ordered_json& schema )ε;

		α Create( const Syntax& syntax )Ι->string;
		α DataTypeString( const Syntax& syntax )Ι->SchemaName;
		α DefaultObject()Ι->DB::object;

		SchemaName Name;
		uint OrdinalPosition;
		string Default;
		bool IsNullable{};
		mutable bool IsFlags{};
		mutable bool IsEnum{};
		mutable const Table* TablePtr{};
		EType Type{ EType::UInt };
		optional<uint> MaxLength;
		bool IsIdentity{};
		bool IsId{};
		optional<uint> NumericPrecision;
		optional<uint> NumericScale;
		bool Insertable{ true };
		bool Updateable{ true };
		SchemaName PKTable;
		string QLAppend;//also select this column in ql query
		string Criteria;//unUsers=not is_group
	};

	struct  Γ Index{
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
	struct Γ Table{
		Table( sv schema, sv name )ι:Schema{schema}, Name{name}{}
		Table( sv name, const nlohmann::json& j, const flat_map<SchemaName,Table>& parents, const flat_map<SchemaName,Column>& commonColumns, const nlohmann::ordered_json& schema )ε;

		α Create( const Syntax& syntax )Ι->string;
		α InsertProcName()Ι->SchemaName;
		α InsertProcText( const Syntax& syntax )Ι->string;
		α FindColumn( sv name )Ι->const Column*;
		α FindColumn( sv name, const DB::Schema& schema )Ε->const Column&;//also looks into extended from table

		α IsFlags()Ι->bool{ return FlagsData.size(); }
		α IsEnum()Ι->bool;//GraphQL attribute
		α GetExtendedFromTable( const DB::Schema& schema )Ι->sp<const Table>;//um_users return um_entities
		α NameWithoutType()Ι->sv;//users in um_users.
		α Prefix()Ι->sv;//um in um_users.
		α JsonTypeName()Ι->string;

		α FKName()Ι->SchemaName;
		bool IsMap( const DB::Schema& schema )Ι{ return ChildTable(schema) && ParentTable(schema); }
		α ChildColumn()Ι->sp<Column>{ return get<1>(ParentChildMap); }
		α ParentColumn()Ι->sp<Column>{ return get<0>(ParentChildMap); }
		α SurrogateKey()Ε->const Column&;
		α ChildTable( const DB::Schema& schema )Ι->sp<const Table>;
		α ParentTable( const DB::Schema& schema )Ι->sp<const Table>;

		bool HaveSequence()Ι{ return std::find_if( Columns.begin(), Columns.end(), [](const auto& c){return c.IsIdentity;} )!=Columns.end(); }
		SchemaName Schema;
		SchemaName Name;
		vector<Column> Columns;
		vector<Index> Indexes;
		vector<SchemaName> SurrogateKeys;
		vector<vector<SchemaName>> NaturalKeys;
		flat_map<uint,string> FlagsData;
		vector<nlohmann::json> Data;
		bool CustomInsertProc{};
		bool IsView{};
		tuple<sp<Column>,sp<Column>> ParentChildMap;//groups entity_id, member_id
		SchemaName PurgeProcName;
		SchemaName QLView;
	};
	struct ForeignKey{
		Ω Create( sv name, sv columnName, const DB::Table& pkTable, sv foreignTable )ε->string;

		SchemaName Name;
		SchemaName Table;
		vector<SchemaName> Columns;
		SchemaName pkTable;
	};

	struct Procedure{
		SchemaName Name;
	};
}