#include "DataSource.h"

#define var const auto
namespace Jde::DB
{
	α IDataSource::Select( string sql, RowΛ f, SL sl )noexcept(false)->void
	{
		Select( move(sql), f, nullptr, sl );
	}
	α IDataSource::Select( string sql, RowΛ f, const vector<object>& values, SL sl )noexcept(false)->void
	{
		Select( move(sql), f, &values, sl );
	}
	α IDataSource::TrySelect( string sql, RowΛ f, SL sl )noexcept->bool
	{
		return Try( [&]{Select( move(sql), f, sl);} );
	}

	α IDataSource::TryExecute( string sql, SL sl )noexcept->optional<uint>
	{
		optional<uint> result;
		try
		{
			result = Execute( move(sql), sl );
		}
		catch( const IException&  ){}

		return result;
	}
	α IDataSource::TryExecute( string sql, const vector<object>& parameters, SL sl )noexcept->optional<uint>
	{
		optional<uint> result;
		try
		{
			result = Execute( move(sql), parameters, sl );
		}
		catch( const IException&  ){}

		return result;
	}

	α IDataSource::TryExecuteProc( string sql, const vector<object>& parameters, SL sl )noexcept->optional<uint>
	{
		optional<uint> result;
		try
		{
			result = ExecuteProc( move(sql), parameters, sl );
		}
		catch( const IException& ){}

		return result;
	}

	α IDataSource::Catalog( string sql, SL sl )noexcept(false)->string
	{
		string db;
		auto fnctn = [&db]( auto& row ){ row >> db; };
		Select( move(sql), fnctn, nullptr, sl );
		return db;
	}
}