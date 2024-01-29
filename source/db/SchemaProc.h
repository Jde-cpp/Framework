#pragma once
#include "types/Schema.h"
#include "types/Table.h"

namespace Jde::DB
{
	struct IDataSource;
	struct ISchemaProc
	{
		ISchemaProc( sp<IDataSource> pDataSource ):_pDataSource{pDataSource}{}
		Γ α CreateSchema( const nlohmann::ordered_json& json, path relativePath )ε->Schema;
		β LoadTables( sv catalog={} )ε->flat_map<string,Table> = 0;
		β LoadProcs( sv catalog={} )ε->flat_map<string,Procedure> = 0;
		β ToType( sv typeName )ι->EType=0;
		β LoadIndexes( sv schema={}, sv tableName={} )ε->vector<Index> = 0;
		β LoadForeignKeys( sv catalog={} )ε->flat_map<string,ForeignKey> = 0;
	protected:
		sp<IDataSource> _pDataSource;
	};
}