#pragma once
#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H
#include <jde/Log.h>
#include "Await.h"
#include "Row.h"
#include "SchemaProc.h"


#define DBLOG(sql,params) Jde::DB::Log( sql, params, sl )
namespace Jde::DB
{
	namespace Types{ struct IRow; }
	struct Γ IDataSource : std::enable_shared_from_this<IDataSource>
	{
		virtual ~IDataSource(){}//warning
		template<class K=uint,class V=string> α SelectEnum( sv tableName, SRCE )ι->SelectCacheAwait<flat_map<K,V>>{ return SelectMap<K,V>( Jde::format("select id, name from {}", tableName), string{tableName}, sl ); }
		ẗ SelectEnumSync( sv tableName, SRCE )ε->sp<flat_map<K,V>>{ ASSERT(tableName.size()); return SFuture<flat_map<K,V>>( SelectEnum<K,V>( tableName, sl) ).get(); }
		ẗ SelectMap( string sql, SRCE )ι->SelectAwait<flat_map<K,V>>;
		ẗ SelectMap( string sql, string cacheName, SRCE )ι->SelectCacheAwait<flat_map<K,V>>;
		Ŧ SelectSet( string sql, vector<object>&& params, SL sl )ι->SelectAwait<flat_set<T>>;
		Ŧ SelectSet( string sql, vector<object>&& params, string cacheName, SL sl )ι->SelectCacheAwait<flat_set<T>>;

		β SchemaProc()ι->sp<ISchemaProc> =0;
		α ScalerNonNull( string sql, vec<object> parameters, SRCE )ε->uint;

		Ŧ TryScaler( string sql, vec<object> parameters, SRCE )ι->optional<T>;
		Ŧ Scaler( string sql, vec<object> parameters, SRCE )ε->optional<T>;
		Ŧ ScalerCo( string sql, vec<object> parameters, SRCE )ε->SelectAwait<T>;
		Ŧ SelectCo( string sql, vec<object> params, CoRowΛ<T> fnctn, SRCE )ε->SelectAwait<T>;

		α TryExecute( string sql, SRCE )ι->optional<uint>;
		α TryExecute( string sql, vec<object> parameters, SRCE )ι->optional<uint>;
		α TryExecuteProc( string sql, vec<object> parameters, SRCE )ι->optional<uint>;

		β Execute( string sql, SRCE )ε->uint=0;
		β Execute( string sql, vec<object> parameters, SRCE )ε->uint=0;
		β Execute( string sql, const vector<object>* pParameters, const RowΛ* f, bool isStoredProc=false, SRCE )ε->uint=0;
		β ExecuteCo( string sql, vector<object> p, SRCE )ι->up<IAwait> =0;
		β ExecuteNoLog( string sql, const vector<object>* pParameters, RowΛ* f=nullptr, bool isStoredProc=false, SRCE )ε->uint=0;
		β ExecuteProc( string sql, vec<object> parameters, SRCE )ε->uint=0;
		β ExecuteProc( string sql, vec<object> parameters, RowΛ f, SRCE )ε->uint=0;
		β ExecuteProcCo( string sql, vector<object> p, SRCE )ι->up<IAwait> =0;
		β ExecuteProcCo( string sql, vector<object> parameters, RowΛ f, SRCE )ε->up<IAwait> =0;
		β ExecuteProcNoLog( string sql, vec<object> parameters, SRCE )ε->uint=0;

		α Select( string sql, RowΛ f, vec<object> parameters, SRCE )ε->void;
		α Select( string sql, RowΛ f, SRCE )ε->void;
		β Select( string sql, RowΛ f, const vector<object>* pValues, SRCE )ε->uint=0;
		β SelectNoLog( string sql, RowΛ f, const vector<object>* pValues, SRCE )ε->uint=0;
		α TrySelect( string sql, RowΛ f, SRCE )ι->bool;
		α Catalog( string sql, SRCE )ε->string;

		α CS()const ι->str{ return _connectionString; }
		β SetConnectionString( string x )ι->void{ _connectionString = move(x); }

		//bool Asynchronous{false};

	protected:
		string _connectionString;
	private:
		β SelectCo( ISelect* pAwait, string sql, vector<object>&& params, SRCE )ι->up<IAwait> =0;
		friend struct ISelect;
	};
#define var const auto
	Ŧ IDataSource::TryScaler( string sql, vec<object> params, SL sl )ι->optional<T>
	{
		try
		{
			return Scaler<T>( move(sql), params, sl );
		}
		catch( IException& )
		{
			return nullopt;
		}
	}
	Ŧ IDataSource::Scaler( string sql, vec<object> parameters, SL sl )ε->optional<T>
	{
		optional<T> result;
		Select( move(sql), [&result](const IRow& row){ result = row.Get<T>(0); }, parameters, sl );
		return result;
	}

	Ŧ IDataSource::ScalerCo( string sql, vec<object> params, SL sl )ε->SelectAwait<T>
	{
		auto f = []( T& y, const IRow& r ){ y = r.Get<T>( 0 ); };
		return SelectAwait<T>( shared_from_this(), move(sql), f, params, sl );
	}
	Ŧ IDataSource::SelectCo( string sql, vec<object> params, CoRowΛ<T> f, SL sl )ε->SelectAwait<T>
	{
		//auto f = []( T& y, const IRow& r ){ y = r.Get<T>( 0 ); };
		return SelectAwait<T>( shared_from_this(), move(sql), f, params, sl );
	}

	namespace zInternal
	{
		ẗ ProcessMapRow( flat_map<K,V>& y, const IRow& row )ε{ y.emplace( row.Get<K>(0), row.Get<V>(1) ); }
		Ŧ ProcessSetRow( flat_set<T>& y, const IRow& row )ε{ y.emplace( row.Get<T>(0) ); }
	}

	ẗ IDataSource::SelectMap( string sql, SL sl )ι->SelectAwait<flat_map<K,V>>
	{
		return SelectAwait<flat_map<K,V>>( shared_from_this(), move(sql), zInternal::ProcessMapRow<K,V>, {}, sl );
	}
	ẗ IDataSource::SelectMap( string sql, string cacheName, SL sl )ι->SelectCacheAwait<flat_map<K,V>>
	{
		return SelectCacheAwait<flat_map<K,V>>( shared_from_this(), move(sql), move(cacheName), zInternal::ProcessMapRow<K,V>, {}, sl );
	}
	Ŧ IDataSource::SelectSet( string sql, vector<object>&& params, SL sl )ι->SelectAwait<flat_set<T>>
	{
		return SelectAwait<flat_set<T>>( shared_from_this(), move(sql), zInternal::ProcessSetRow<T>, move(params), sl );
	}
	Ŧ IDataSource::SelectSet( string sql, vector<object>&& params, string cacheName, SL sl )ι->SelectCacheAwait<flat_set<T>>
	{
		return SelectCacheAwait<flat_set<T>>( shared_from_this(), move(sql), move(cacheName), zInternal::ProcessSetRow<T>, move(params), sl );
	}
	Ξ ISelect::SelectCo( string sql, vector<object>&& params, SL sl )ι->up<IAwait>
	{
		return _ds->SelectCo( this, move(sql), move(params), sl );
	}
}
#undef var
#endif