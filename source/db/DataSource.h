#pragma once
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
		ⓣ SelectEnum( str tableName, SRCE )noexcept{ return SelectMap<T,string>( format("select id, name from {}", tableName), tableName, sl ); }
		ⓣ SelectEnumSync( str tableName, SRCE )noexcept->sp<flat_map<T,string>>{ ASSERT(tableName.size()); return SFuture<flat_map<T,string>>( SelectEnum<T>( move(tableName), sl) ).get(); }
		ẗ SelectMap( string sql, SRCE )noexcept->SelectAwait<flat_map<K,V>>;
		ẗ SelectMap( string sql, string cacheName, SRCE )noexcept->SelectCacheAwait<flat_map<K,V>>;
		ⓣ SelectSet( string sql, vector<object>&& params, SL sl )noexcept->SelectAwait<flat_set<T>>;
		ⓣ SelectSet( string sql, vector<object>&& params, string cacheName, SL sl )noexcept->SelectCacheAwait<flat_set<T>>;

		β SchemaProc()noexcept->sp<ISchemaProc> =0;
		α ScalerNonNull( string sql, vec<object> parameters, SRCE )noexcept(false)->uint;

		ⓣ TryScaler( string sql, vec<object> parameters, SRCE )noexcept->optional<T>;
		ⓣ Scaler( string sql, vec<object> parameters, SRCE )noexcept(false)->optional<T>;
		ⓣ ScalerCo( string sql, vec<object> parameters, SRCE )noexcept(false)->SelectAwait<T>;

		α TryExecute( string sql, SRCE )noexcept->optional<uint>;
		α TryExecute( string sql, vec<object> parameters, SRCE )noexcept->optional<uint>;
		α TryExecuteProc( string sql, vec<object> parameters, SRCE )noexcept->optional<uint>;

		β Execute( string sql, SRCE )noexcept(false)->uint=0;
		β Execute( string sql, vec<object> parameters, SRCE )noexcept(false)->uint=0;
		β Execute( string sql, const vector<object>* pParameters, RowΛ* f, bool isStoredProc=false, SRCE )noexcept(false)->uint=0;
		β ExecuteNoLog( string sql, const vector<object>* pParameters, RowΛ* f=nullptr, bool isStoredProc=false, SRCE )noexcept(false)->uint=0;
		β ExecuteProc( string sql, vec<object> parameters, SRCE )noexcept(false)->uint=0;
		β ExecuteProc( string sql, vec<object> parameters, RowΛ f, SRCE )noexcept(false)->uint=0;
		β ExecuteProcCo( string sql, vector<object> p, SRCE )noexcept->up<IAwait> =0;
		β ExecuteProcNoLog( string sql, vec<object> parameters, SRCE )noexcept(false)->uint=0;

		α Select( string sql, RowΛ f, vec<object> parameters, SRCE )noexcept(false)->void;
		α Select( string sql, RowΛ f, SRCE )noexcept(false)->void;
		β Select( string sql, RowΛ f, const vector<object>* pValues, SRCE )noexcept(false)->uint=0;
		β SelectNoLog( string sql, RowΛ f, const vector<object>* pValues, SRCE )noexcept(false)->uint=0;
		α TrySelect( string sql, RowΛ f, SRCE )noexcept->bool;
		α Catalog( string sql, SRCE )noexcept(false)->string;

		α CS()const noexcept->str{ return _connectionString; }
		β SetConnectionString( string x )noexcept->void{ _connectionString = move(x); }

		//bool Asynchronous{false};

	protected:
		string _connectionString;
	private:
		β SelectCo( ISelect* pAwait, string sql, vector<object>&& params, SRCE )noexcept->up<IAwait> =0;
		friend struct ISelect;
	};
#define var const auto
	ⓣ IDataSource::TryScaler( string sql, vec<object> params, SL sl )noexcept->optional<T>
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
	ⓣ IDataSource::Scaler( string sql, vec<object> parameters, SL sl )noexcept(false)->optional<T>
	{
		optional<T> result;
		Select( move(sql), [&result](const IRow& row){ result = row.Get<T>(0); }, parameters, sl );
		return result;
	}

	ⓣ IDataSource::ScalerCo( string sql, vec<object> params, SL sl )noexcept(false)->SelectAwait<T>
	{
		auto f = []( T& y, const IRow& r ){ y = r.Get<T>( 0 ); };
		return SelectAwait<T>( shared_from_this(), move(sql), f, params, sl );
	}

	namespace zInternal
	{
		ẗ ProcessMapRow( flat_map<K,V>& y, const IRow& row )noexcept(false){ y.emplace( row.Get<K>(0), row.Get<V>(1) ); }
		ⓣ ProcessSetRow( flat_set<T>& y, const IRow& row )noexcept(false){ y.emplace( row.Get<T>(0) ); }
	}

	ẗ IDataSource::SelectMap( string sql, SL sl )noexcept->SelectAwait<flat_map<K,V>>
	{
		return SelectAwait<flat_map<K,V>>( shared_from_this(), move(sql), zInternal::ProcessMapRow<K,V>, {}, sl );
	}
	ẗ IDataSource::SelectMap( string sql, string cacheName, SL sl )noexcept->SelectCacheAwait<flat_map<K,V>>
	{
		return SelectCacheAwait<flat_map<K,V>>( shared_from_this(), move(sql), move(cacheName), zInternal::ProcessMapRow<K,V>, {}, sl );
	}
	ⓣ IDataSource::SelectSet( string sql, vector<object>&& params, SL sl )noexcept->SelectAwait<flat_set<T>>
	{
		return SelectAwait<flat_set<T>>( shared_from_this(), move(sql), zInternal::ProcessSetRow<T>, move(params), sl );
	}
	ⓣ IDataSource::SelectSet( string sql, vector<object>&& params, string cacheName, SL sl )noexcept->SelectCacheAwait<flat_set<T>>
	{
		return SelectCacheAwait<flat_set<T>>( shared_from_this(), move(sql), move(cacheName), zInternal::ProcessSetRow<T>, move(params), sl );
	}
	Ξ ISelect::SelectCo( string sql, vector<object>&& params, SL sl )noexcept->up<IAwait>
	{
		return _ds->SelectCo( this, move(sql), move(params), sl );
	}
}
#undef var