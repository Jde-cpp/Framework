#pragma once
#include "../Exports.h"
#include "DataSource.h"
#include "../Cache.h"

#define QUERY if( var pDS = Jde::DB::DataSource(); pDS ) (*pDS)
namespace Jde::DB
{
	struct IDataSource; struct Syntax; //struct DataValue;
	void Log( sv sql, const std::vector<DataValue>* pParameters, sv file, sv fnctn, uint line, ELogLevel level=ELogLevel::Debug, sv error={} )noexcept;
	JDE_NATIVE_VISIBILITY sp<Syntax> DefaultSyntax()noexcept;
	JDE_NATIVE_VISIBILITY sp<IDataSource> DataSource()noexcept(false);
	JDE_NATIVE_VISIBILITY sp<IDataSource> DataSource( path libraryName, sv connectionString )noexcept(false);
	JDE_NATIVE_VISIBILITY void CreateSchema()noexcept(false);
	JDE_NATIVE_VISIBILITY void CleanDataSources()noexcept;
	JDE_NATIVE_VISIBILITY void ShutdownClean( function<void()>& shutdown )noexcept;

	JDE_NATIVE_VISIBILITY uint ExecuteProc( sv sql, std::vector<DataValue>&& parameters )noexcept(false);
	JDE_NATIVE_VISIBILITY uint Execute( sv sql, std::vector<DataValue>&& parameters )noexcept(false);

	template<typename T> optional<T> TryScaler( sv sql, const vector<DataValue>& parameters )noexcept;
	template<typename T> optional<T> Scaler( sv sql, const vector<DataValue>& parameters )noexcept;

	JDE_NATIVE_VISIBILITY void Select( sv sql, std::function<void(const IRow&)> f, const vector<DataValue>& values )noexcept(false);
	JDE_NATIVE_VISIBILITY void Select( sv sql, std::function<void(const IRow&)> f )noexcept(false);
	template<typename K,typename V> flat_map<K,V> SelectMap( sv sql )noexcept(false);
	template<typename K,typename V> flat_map<K,V> SelectMap( sv sql, str cacheName )noexcept(false);
	template<typename T> boost::container::flat_set<T> SelectSet( sv sql, const std::vector<DataValue>& parameters )noexcept(false);

	CIString SelectName( sv sql, uint id, sv cacheName )noexcept(false);
}

namespace Jde
{
	using boost::container::flat_set;
#define RETURN(x) auto pDataSource = DataSource(); return !pDataSource ? x : pDataSource

	template<typename T> optional<T> DB::Scaler( sv sql, const vector<DataValue>& parameters )noexcept
	{
		RETURN( optional<T>{} )->Scaler<T>( sql, parameters );
	}
	template<typename T> optional<T> DB::TryScaler( sv sql, const vector<DataValue>& parameters )noexcept
	{
		RETURN( optional<T>{} )->TryScaler<T>( sql, parameters );
	}

	template<typename K,typename V>
	flat_map<K,V> DB::SelectMap( sv sql )noexcept(false)
	{
		auto p = DataSource();
		return p ? p->SelectMap<K,V>( sql ) : flat_map<K,V>{};
		//RETURN( flat_map<K,V>{} )->SelectMap<K,V>( sql );
	}
#define var const auto
	template<typename K,typename V>
	flat_map<K,V> DB::SelectMap( sv sql, str cacheName )noexcept(false)
	{
		flat_map<K,V> result;
		if( var p = Cache::Get<flat_map<K,V>>(cacheName); p )
			for_each( p->begin(), p->end(), [&result](var& x){ result.emplace(x.first, x.second); } );
		else
		{
			result = SelectMap<K,V>( sql );
			Cache::Set( cacheName, make_shared<flat_map<K,V>>(result) );
		}
		return result;
	}
	template<typename T>
	flat_set<T> DB::SelectSet( sv sql, const std::vector<DataValue>& parameters )noexcept(false)
	{
		flat_set<T> results;
		auto fnctn = [&results]( const IRow& row ){ results.emplace(row.Get<T>(0)); };
		Select( sql, fnctn, parameters );
		return results;
	}

#undef var
}