#include "DataSource.h"

#define var const auto
namespace Jde::DB
{
	void IDataSource::Select( sv sql, std::function<void(const IRow&)> f )
	{
		Select( sql, f, nullptr, true );
	}
	void IDataSource::Select( sv sql, std::function<void(const IRow&)> f, const vector<DataValue>& values, bool log, const source_location& sl )noexcept(false)
	{
		Select( sql, f, &values, log, sl );
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
		catch( const IException&  )
		{}
		return result;
	}
	optional<uint> IDataSource::TryExecute( sv sql, const vector<DataValue>& parameters, bool log )noexcept
	{
		optional<uint> result;
		try
		{
			result = Execute( sql, parameters, log );
		}
		catch( const IException&  )
		{}

		return result;
	}

	optional<uint> IDataSource::TryExecuteProc( sv sql, const vector<DataValue>& parameters, bool log )noexcept
	{
		optional<uint> result;
		try
		{
			result = ExecuteProc( sql, parameters, log );
		}
		catch( const IException& )
		{}

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