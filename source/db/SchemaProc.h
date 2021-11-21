#pragma once
#include "types/Schema.h"
#include "types/Table.h"

namespace Jde::DB
{
	struct IDataSource;
	struct ISchemaProc
	{
		ISchemaProc( sp<IDataSource> pDataSource ):_pDataSource{pDataSource}{}
		Γ α CreateSchema( const nlohmann::ordered_json& json, path relativePath )noexcept(false)->Schema;
		β LoadTables( sv catalog={} )noexcept(false)->up<flat_map<string,Table>> = 0;
		β LoadProcs( sv catalog={} )noexcept(false)->flat_map<string,Procedure> = 0;
		β ToType( sv typeName )noexcept->EType=0;
		β LoadIndexes( sv schema={}, sv tableName={} )noexcept(false)->vector<Index> = 0;
		β LoadForeignKeys( sv catalog={} )noexcept(false)->flat_map<string,ForeignKey> = 0;
	protected:
		sp<IDataSource> _pDataSource;
	};
}