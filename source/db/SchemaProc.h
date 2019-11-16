#pragma once
#include "types/Schema.h"
#include "types/Table.h"

namespace Jde::DB
{
	struct IDataSource;
	struct ISchemaProc
	{
		virtual MapPtr<string,Types::Table> LoadTables( IDataSource& ds, string_view catalog )noexcept(false)=0;
		virtual DataType ToDataType( string_view typeName )noexcept=0;
		//virtual Types::Table LoadTable( IDataSource& ds, string_view catalog, string_view tableName )noexcept(false)=0;
		//virtual Types::Schema LoadSchema( IDataSource& ds, string_view catalog )noexcept(false)=0;
	};
}