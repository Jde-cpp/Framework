#pragma once
#include "types/Schema.h"
#include "types/Table.h"

namespace Jde::DB
{
	struct IDataSource;
	struct ISchemaProc
	{
		ISchemaProc( sp<IDataSource> pDataSource ):_pDataSource{pDataSource}{}
		JDE_NATIVE_VISIBILITY Schema CreateSchema( const nlohmann::ordered_json& json )noexcept(false);
		virtual MapPtr<string,Table> LoadTables( sv catalog={} )noexcept(false)=0;
		virtual flat_map<string,Procedure> LoadProcs( sv catalog={} )noexcept(false)=0;
		virtual DataType ToDataType( sv typeName )noexcept=0;
		virtual vector<Index> LoadIndexes( sv schema={}, sv tableName={} )noexcept(false)=0;
		virtual flat_map<string,ForeignKey> LoadForeignKeys( sv catalog={} )noexcept(false)=0;
	protected:
		sp<IDataSource> _pDataSource;
		//virtual Types::Table LoadTable( IDataSource& ds, sv catalog, sv tableName )noexcept(false)=0;
		//virtual Types::Schema LoadSchema( IDataSource& ds, sv catalog )noexcept(false)=0;
	};
}