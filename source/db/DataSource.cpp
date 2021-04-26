#include "DataSource.h"

#define var const auto
namespace Jde::DB
{
/*	uint IDataSource::Scaler( sv sql, const vector<DataValue>& parameters )noexcept(false)
	{
		uint count = 0;
		function<void(const IRow&)> fnctn = [&count](const IRow& row){ row >> count; };
		Execute( sql, &parameters, &fnctn, false, true );
		return count;
	}
	optional<uint> IDataSource::ScalerOptional( sv sql, const vector<DataValue>& parameters )noexcept(false)
	{
		optional<uint> value;
		function<void(const IRow&)> f = [&value](var& row){ value = row.GetUIntOpt(0); };
		Execute( sql, &parameters, &f, false, true );
		return value;
	}*/

	void IDataSource::Select( sv sql, std::function<void(const IRow&)> f )
	{
		Select( sql, f, nullptr, true );
	}
	void IDataSource::Select( sv sql, std::function<void(const IRow&)> f, const vector<DataValue>& values, bool log )noexcept(false)
	{
		Select( sql, f, &values, log );
	}
	bool IDataSource::TrySelect( sv sql, std::function<void(const IRow&)> f )noexcept
	{
		return Try( [&]{Select( sql, f);} );
	}

	optional<uint> IDataSource::TryExecute( sv sql )noexcept
	{
		optional<uint> result;
		try
		{
			result = Execute( sql );
		}
		catch( const Exception& e )
		{
			e.Log();
		}
		return result;
	}
	optional<uint> IDataSource::TryExecute( sv sql, const vector<DataValue>& parameters, bool log )noexcept
	{
		optional<uint> result;
		try
		{
			result = Execute( sql, parameters, log );
		}
		catch( const Exception& e )
		{
			e.Log();
		}
		return result;
	}

	optional<uint> IDataSource::TryExecuteProc( sv sql, const vector<DataValue>& parameters, bool log )noexcept
	{
		optional<uint> result;
		try
		{
			result = ExecuteProc( sql, parameters, log );
		}
		catch( const Exception& e )
		{
			e.Log();
		}
		return result;
	}

	string IDataSource::Catalog( sv sql )noexcept(false)
	{
		string db;
		auto fnctn = [&db]( auto& row ){ row >> db; };
		Select( sql, fnctn, {}, false );
		return db;
	}
}