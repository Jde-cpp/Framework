#pragma once
#include "../Exports.h"
#include "DataSource.h"

#define QUERY if( var pDS = Jde::DB::DataSource(); pDS ) (*pDS)
namespace Jde::DB
{
	JDE_NATIVE_VISIBILITY shared_ptr<IDataSource> DataSource();
	JDE_NATIVE_VISIBILITY shared_ptr<IDataSource> DataSource( path libraryName, string_view connectionString );
	JDE_NATIVE_VISIBILITY void CleanDataSources()noexcept;
}