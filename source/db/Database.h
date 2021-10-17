#pragma once
#include <jde/Exports.h>
#include "DataSource.h"
#include "../Cache.h"

#define QUERY if( var pDS = Jde::DB::DataSource(); pDS ) (*pDS)
#define 🚪 JDE_NATIVE_VISIBILITY auto
namespace Jde::DB
{
	struct IDataSource; struct Syntax; //struct DataValue;
	α Message( sv sql, const std::vector<DataValue>* pParameters, sv error={} )noexcept->string;
	α Log( sv sql, const std::vector<DataValue>* pParameters, sv file, sv fnctn, int line )noexcept->void;
	α Log( sv sql, const std::vector<DataValue>* pParameters, sv file, sv fnctn, int line, ELogLevel level, sv error )noexcept->void;
	🚪 DefaultSyntax()noexcept->sp<Syntax>;
	🚪 DataSource()noexcept(false)->sp<IDataSource>;
	🚪 DataSource( path libraryName, sv connectionString )noexcept(false)->sp<IDataSource>;
	🚪 CreateSchema()noexcept(false)->void;
	🚪 CleanDataSources()noexcept->void;
	🚪 ShutdownClean( function<void()>& shutdown )noexcept->void;

	🚪 ExecuteProc( sv sql, std::vector<DataValue>&& parameters )noexcept(false)->uint;
	🚪 Execute( sv sql, std::vector<DataValue>&& parameters )noexcept(false)->uint;

	ⓣ TryScaler( sv sql, const vector<DataValue>& parameters )noexcept->optional<T>;
	ⓣ Scaler( sv sql, const vector<DataValue>& parameters )noexcept(false)->optional<T>;
	ⓣ ScalerCo( string&& sql, const vector<DataValue>&& parameters )noexcept(false);

	🚪 Select( sv sql, std::function<void(const IRow&)> f, const vector<DataValue>& values )noexcept(false)->void;
	🚪 Select( sv sql, std::function<void(const IRow&)> f )noexcept(false)->void;
	🚪 SelectIds( sv sql, const set<uint>& ids, std::function<void(const IRow&)> f )noexcept(false)->void;

	template<class K,class V> sp<flat_map<K,V>> SelectMap( sv sql, str cacheName={} )noexcept(false);
	ⓣ SelectSet( sv sql, const std::vector<DataValue>& parameters )noexcept(false)->boost::container::flat_set<T>;
	ⓣ SelectSet( sv sql, str cacheName, const std::vector<DataValue>& parameters )noexcept(false)->sp<boost::container::flat_set<T>>;

	🚪 SelectName( sv sql, uint id, sv cacheName )noexcept(false)->CIString;
}

namespace Jde
{
	using boost::container::flat_set;
#define RETURN(x) auto pDataSource = DataSource(); return !pDataSource ? x : pDataSource

	template<class T> optional<T> DB::Scaler( sv sql, const vector<DataValue>& parameters )noexcept(false)
	{
		RETURN( optional<T>{} )->Scaler<T>( sql, parameters );
	}
	template<class T> optional<T> DB::TryScaler( sv sql, const vector<DataValue>& parameters )noexcept
	{
		RETURN( optional<T>{} )->TryScaler<T>( sql, parameters );
	}
#define var const auto
	template<class K,class V> sp<flat_map<K,V>> DB::SelectMap( sv sql, str cacheName )noexcept(false)
	{
		auto selectMap = [sql]()
		{
			auto p = DataSource();
			return p ? p->SelectMap<K,V>( sql ) : make_shared<flat_map<K,V>>();
		};
		sp<flat_map<K,V>> p = cacheName.size() ? Cache::Get<flat_map<K,V>>( cacheName ) : selectMap();
		if( !p )
			Cache::Set( cacheName, p = selectMap() );
		return p;
	}
	template<class T> flat_set<T> DB::SelectSet( sv sql, const std::vector<DataValue>& parameters )noexcept(false)
	{
		flat_set<T> results;
		auto fnctn = [&results]( const IRow& row ){ results.emplace(row.Get<T>(0)); };
		Select( sql, fnctn, parameters );
		return results;
	}
	ⓣ DB::SelectSet( sv sql, str cacheName, const std::vector<DataValue>& parameters )noexcept(false)->sp<boost::container::flat_set<T>>
	{
		auto p = cacheName.size() ? Cache::Get<boost::container::flat_set<T>>( cacheName ) : make_shared<flat_set<T>>(SelectSet<T>(sql, parameters) );
		if( !p )
			Cache::Set( cacheName, p = make_shared<flat_set<T>>(SelectSet<T>(sql, parameters)) );
		return p;
	}

#undef var
#undef  🚪
}