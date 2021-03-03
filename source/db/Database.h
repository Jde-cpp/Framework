#pragma once
#include "../Exports.h"
#include "DataSource.h"

#define QUERY if( var pDS = Jde::DB::DataSource(); pDS ) (*pDS)
namespace Jde::DB
{
	struct IDataSource; struct Syntax; //struct DataValue;
	JDE_NATIVE_VISIBILITY sp<Syntax> DefaultSyntax()noexcept;
	JDE_NATIVE_VISIBILITY sp<IDataSource> DataSource()noexcept(false);
	JDE_NATIVE_VISIBILITY sp<IDataSource> DataSource( path libraryName, string_view connectionString );
	JDE_NATIVE_VISIBILITY void CleanDataSources()noexcept;

	template<typename T>
	optional<T> TryScaler( sv sql, const vector<DataValue>& parameters )noexcept;
}

namespace Jde
{
	template<typename T>
	optional<T> DB::TryScaler( sv sql, const vector<DataValue>& parameters )noexcept
	{
		auto p = DataSource();
		return p ? p->TryScaler<T>( sql, parameters ) : optional<T>{};
	}
}