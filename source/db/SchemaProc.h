#pragma once
#include "types/Schema.h"
#include "types/Table.h"

namespace Jde::DB
{
	struct IDataSource;
	struct ISchemaProc
	{
		ISchemaProc( sp<IDataSource> pDataSource ):_pDataSource{pDataSource}{}
		Schema CreateSchema( const nlohmann::json& json )noexcept(false);
		virtual MapPtr<string,Table> LoadTables( string_view catalog={} )noexcept(false)=0;
		virtual flat_map<string,Procedure> LoadProcs( string_view catalog={} )noexcept(false)=0;
		virtual DataType ToDataType( string_view typeName )noexcept=0;
		virtual vector<Index> LoadIndexes( sv schema={}, sv tableName={} )noexcept(false)=0;
		virtual flat_map<string,ForeignKey> LoadForeignKeys( sv catalog={} )noexcept(false)=0;
	protected:
		sp<IDataSource> _pDataSource;
		//virtual Types::Table LoadTable( IDataSource& ds, string_view catalog, string_view tableName )noexcept(false)=0;
		//virtual Types::Schema LoadSchema( IDataSource& ds, string_view catalog )noexcept(false)=0;
	};
}