﻿#include "DataSource.h"

#define var const auto
namespace Jde::DB{
	α IDataSource::Select( string sql, RowΛ f, SL sl )ε->void{
		Select( move(sql), f, nullptr, sl );
	}
	α IDataSource::Select( string sql, RowΛ f, const vector<object>& values, SL sl )ε->void{
		Select( move(sql), f, &values, sl );
	}
	α IDataSource::TrySelect( string sql, RowΛ f, SL sl )ι->bool{
		return Try( [&]{Select( move(sql), f, sl);} );
	}

	α IDataSource::TryExecute( string sql, SL sl )ι->optional<uint>{
		optional<uint> result;
		try{
			result = Execute( move(sql), sl );
		}
		catch( const IException&  ){}

		return result;
	}
	α IDataSource::TryExecute( string sql, const vector<object>& parameters, SL sl )ι->optional<uint>{
		optional<uint> result;
		try{
			result = Execute( move(sql), parameters, sl );
		}
		catch( const IException&  ){}

		return result;
	}

	α IDataSource::TryExecuteProc( string sql, const vector<object>& parameters, SL sl )ι->optional<uint>{
		optional<uint> result;
		try{
			result = ExecuteProc( move(sql), parameters, sl );
		}
		catch( const IException& ){}

		return result;
	}

	α IDataSource::Catalog( string sql, SL sl )ε->string{
		string db;
		auto fnctn = [&db]( auto& row ){ row >> db; };
		Select( move(sql), fnctn, nullptr, sl );
		return db;
	}
}