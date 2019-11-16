#pragma once
#include "../DataType.h"

namespace Jde::DB::Types
{
	struct SurrogateKey
	{};
	struct Column
	{
		Column()=default;
		Column( string_view name, uint ordinalPosition, string_view dflt, bool isNullable, DataType type, optional<uint> maxLength, bool isIdentity, bool isId, optional<uint> numericPrecision, optional<uint> numericScale ):
			Name{name},
			OrdinalPosition{ordinalPosition},
			Default{dflt},
			IsNullable{isNullable},
			Type{type},
			MaxLength{maxLength},
			IsIdentity{isIdentity},
			IsId{isId},
			NumericPrecision{numericPrecision}, 
			NumericScale{numericScale}
		{}
		Column( string_view name ):
			Name{name}
		{}

		string Name;
		uint OrdinalPosition;
		string Default;
		bool IsNullable;
		DataType Type;
		optional<uint> MaxLength;
		bool IsIdentity;
		bool IsId;
		optional<uint> NumericPrecision;
		optional<uint> NumericScale;
		std::optional<SurrogateKey> SrrgtKey;
	};
	struct Table;
	struct Index
	{
		Index( const shared_ptr<Table>& pTable, string_view indexName, bool clustered, bool unique, bool primaryKey );
		string Name;
		string TableName;
		shared_ptr<Table> TablePtr;
		vector<shared_ptr<Column>> Columns;
		bool Clustered;
		bool Unique;
		bool PrimaryKey;
	};
	struct Table
	{
		Table( string_view schema, string_view name ):
			Schema{schema},
			Name{name}
		{}
		sp<Column> FindColumn( string_view name );
		string Schema;
		string Name;
		vector<sp<Column>> Columns;
		vector<Index> Indexes;
	};
	typedef shared_ptr<Table> TablePtr_;
}