#pragma once
#include <jde/Exports.h>
#include "DataSource.h"
#include "../Cache.h"
#define Φ Γ auto
namespace Jde::DB
{
	struct IDataSource; struct Syntax;
	Φ DataSource()noexcept(false)->IDataSource&;
	Φ DataSource( path libraryName, string connectionString )noexcept(false)->sp<IDataSource>;
	Φ DataSourcePtr()noexcept(false)->sp<IDataSource>;
	ⓣ SelectEnum( sv tableName, SRCE )noexcept(false)->up<IAwait>{ return DataSource().SelectEnum<T>( tableName ); }//sp<flat_map<T,string>>
	template<class K=uint,class V=string> α SelectEnumSync( sv tableName, SRCE )noexcept(false)->sp<flat_map<K,V>>{ return DataSource().SelectEnumSync<K,V>( tableName ); }//sp<flat_map<T,string>>
	Φ IdFromName( sv tableName, string name, SRCE )noexcept->SelectAwait<uint>;

	Φ LogDisplay( sv sql, const vector<object>* pParameters, string error={} )noexcept->string;
	Φ Log( sv sql, const vector<object>* pParameters, SL sl )noexcept->void;
	α Log( sv sql, const vector<object>* pParameters, ELogLevel level, string error, SL sl )noexcept->void;
	α LogNoServer( string sql, const vector<object>* pParameters, ELogLevel level, string error, SL sl )noexcept->void;
	Φ DefaultSyntax()noexcept->const DB::Syntax&;
	Φ CreateSchema()noexcept(false)->void;
	Φ DefaultSchema()noexcept(false)->Schema&;
	Φ CleanDataSources()noexcept->void;
	Φ ShutdownClean( function<void()>& shutdown )noexcept->void;

	Ξ ExecuteProc( string sql, vector<object>&& parameters, SRCE )noexcept(false)->uint{ return DataSource().ExecuteProc( move(sql), parameters, sl ); }
	Ξ ExecuteProcCo( string&& sql, vec<object>&& parameters, SRCE )noexcept{ return DataSource().ExecuteProcCo( move(sql), move(parameters), sl ); }
	Φ Execute( string sql, vector<object>&& parameters, SRCE )noexcept(false)->uint;

	ⓣ TryScaler( string sql, vec<object>& parameters, SRCE )noexcept->optional<T>;
	ⓣ Scaler( string sql, vec<object>& parameters, SRCE )noexcept(false)->optional<T>;
	ⓣ ScalerCo( string sql, vec<object> parameters, SRCE )noexcept(false)->SelectAwait<T>;

	Φ Select( string sql, std::function<void(const IRow&)> f, vec<object>& values, SRCE )noexcept(false)->void;
	Φ Select( string sql, std::function<void(const IRow&)> f, SRCE )noexcept(false)->void;
	Φ SelectIds( string sql, const std::set<uint>& ids, std::function<void(const IRow&)> f, SRCE )noexcept(false)->void;

	ẗ SelectMap( string sql, SRCE )noexcept{ return DataSource().SelectMap<K,V>( move(sql), sl ); }
	ẗ SelectMap( string sql, string cacheName, SRCE )noexcept{ return DataSource().SelectMap<K,V>( move(sql), move(cacheName), sl ); }
	ⓣ SelectSet( string sql, vector<object>&& params, SRCE )noexcept{ return DataSource().SelectSet<T>( move(sql), move(params), sl ); }
	ⓣ SelectSet( string sql, vector<object>&& params, string cacheName, SRCE )noexcept{ return DataSource().SelectSet<T>( move(sql), move(params), move(cacheName), sl ); }

	Φ SelectName( string sql, uint id, sv cacheName, SRCE )noexcept(false)->CIString;
}

namespace Jde
{
	using boost::container::flat_set;
	ⓣ DB::Scaler( string sql, vec<object>& parameters, SL sl )noexcept(false)->optional<T>
	{
		return DataSource().Scaler<T>( move(sql), parameters, sl );
	}
	ⓣ DB::TryScaler( string sql, vec<object>& parameters, SL sl )noexcept->optional<T>
	{
		return DataSource().TryScaler<T>( move(sql), parameters, sl );
	}
	ⓣ DB::ScalerCo( string sql, vec<object> parameters, SL sl )noexcept(false)->SelectAwait<T>
	{
		return DataSource().ScalerCo<T>( move(sql), parameters, sl );
	}
#undef  Φ
}