﻿#pragma once
#ifndef DATABASE_H
#define DATABASE_H
#include <jde/Exports.h>
#include "DataSource.h"
#include "../Cache.h"
#define Φ Γ auto
namespace Jde::DB{
	struct IDataSource; struct Syntax;

	Φ SqlTag()ι->sp<LogTag>;
	Φ DataSource()ι->IDataSource&;
	Φ DataSource( const fs::path& libraryName, string connectionString )ε->sp<IDataSource>;
	Φ DataSourcePtr()ε->sp<IDataSource>;
	Φ Driver()ι->string;
	Ŧ SelectEnum( sv tableName, SRCE )ε->SelectCacheAwait<flat_map<T,string>>{ return DataSource().SelectEnum<T>(tableName, sl); }//sp<flat_map<T,string>>
	template<class K=uint,class V=string> α SelectEnumSync( sv tableName, SRCE )ε->sp<flat_map<K,V>>{ return DataSource().SelectEnumSync<K,V>( tableName, sl ); }//sp<flat_map<T,string>>
	Φ IdFromName( sv tableName, string name, SRCE )ι->SelectAwait<uint>;
	Φ ToParamString( uint c )->string;

	Φ LogDisplay( sv sql, const vector<object>* pParameters, string error={} )ι->string;
	Φ Log( sv sql, const vector<object>* pParameters, SL sl )ι->void;
	α Log( sv sql, const vector<object>* pParameters, ELogLevel level, string error, SL sl )ι->void;
	α LogNoServer( string sql, const vector<object>* pParameters, ELogLevel level, string error, SL sl )ι->void;
	Φ DefaultSyntax()ι->const DB::Syntax&;
	Φ CreateSchema()ε->void;
	Φ DefaultSchema()ι->Schema&;
	Φ CleanDataSources( bool terminate )ι->void;
	Φ ShutdownClean( function<void()> shutdown )ι->void;

	Ξ ExecuteProc( string sql, vec<object> parameters, RowΛ f, SRCE )ε->uint{ return DataSource().ExecuteProc( move(sql), parameters, f, sl ); }
	Ξ ExecuteProc( string sql, vector<object>&& parameters, SRCE )ε->uint{ return DataSource().ExecuteProc( move(sql), parameters, sl ); }
	Ξ ExecuteProcCo( string&& sql, vector<object>&& parameters, SRCE )ι{ return DataSource().ExecuteProcCo( move(sql), move(parameters), sl ); }
	Φ Execute( string sql, vector<object>&& parameters, SRCE )ε->uint;
	Ξ ExecuteCo( string&& sql, vector<object>&& parameters, SRCE )ι{ return DataSource().ExecuteCo( move(sql), move(parameters), sl ); }

	Ŧ TryScaler( string sql, vec<object>& parameters, SRCE )ι->optional<T>;
	Ŧ Scaler( string sql, vec<object>& parameters={}, SRCE )ε->optional<T>;
	Ŧ ScalerCo( string sql, vec<object> parameters, SRCE )ε->SelectAwait<T>;

	Φ Select( string sql, std::function<void(const IRow&)> f, vec<object>& values, SRCE )ε->void;
	Ŧ SelectCo( string sql, vec<object> params, CoRowΛ<T> fnctn, SRCE )ε->SelectAwait<T>{ return DataSource().SelectCo<T>( move(sql), params, fnctn, sl ); }

	Φ Select( string sql, std::function<void(const IRow&)> f, SRCE )ε->void;
	Φ SelectIds( string sql, const std::set<uint>& ids, std::function<void(const IRow&)> f, SRCE )ε->void;

	ẗ SelectMap( string sql, SRCE )ι{ return DataSource().SelectMap<K,V>( move(sql), sl ); }
	ẗ SelectMap( string sql, string cacheName, SRCE )ι{ return DataSource().SelectMap<K,V>( move(sql), move(cacheName), sl ); }
	Ŧ SelectSet( string sql, vector<object>&& params, SRCE )ι{ return DataSource().SelectSet<T>( move(sql), move(params), sl ); }
	Ŧ SelectSet( string sql, vector<object>&& params, string cacheName, SRCE )ι{ return DataSource().SelectSet<T>( move(sql), move(params), move(cacheName), sl ); }

	Φ SelectName( string sql, uint id, sv cacheName, SRCE )ε->CIString;
}

namespace Jde{
	using boost::container::flat_set;
	Ŧ DB::Scaler( string sql, vec<object>& parameters, SL sl )ε->optional<T>{
		return DataSource().Scaler<T>( move(sql), parameters, sl );
	}
	Ŧ DB::TryScaler( string sql, vec<object>& parameters, SL sl )ι->optional<T>{
		return DataSource().TryScaler<T>( move(sql), parameters, sl );
	}
	Ŧ DB::ScalerCo( string sql, vec<object> parameters, SL sl )ε->SelectAwait<T>{
		return DataSource().ScalerCo<T>( move(sql), parameters, sl );
	}
#undef  Φ
}
#endif