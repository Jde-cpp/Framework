#pragma once
#include <jde/Exports.h>
#include "DataSource.h"
#include "../Cache.h"

#define QUERY if( var pDS = Jde::DB::DataSource(); pDS ) (*pDS)
namespace Jde::DB
{
	#define 🚪 JDE_NATIVE_VISIBILITY auto
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

	template<class T> optional<T> TryScaler( sv sql, const vector<DataValue>& parameters )noexcept;
	template<class T> optional<T> Scaler( sv sql, const vector<DataValue>& parameters )noexcept;

	JDE_NATIVE_VISIBILITY void Select( sv sql, std::function<void(const IRow&)> f, const vector<DataValue>& values )noexcept(false);
	JDE_NATIVE_VISIBILITY void Select( sv sql, std::function<void(const IRow&)> f )noexcept(false);
	//template<class K,class V> sp<flat_map<K,V>> SelectMapSP( sv sql, str cacheName={} )noexcept(false);
	template<class K,class V> sp<flat_map<K,V>> SelectMap( sv sql, str cacheName={} )noexcept(false);
	ⓣ SelectSet( sv sql, const std::vector<DataValue>& parameters )noexcept(false)->boost::container::flat_set<T>;

	🚪 SelectName( sv sql, uint id, sv cacheName )noexcept(false)->CIString;
}

namespace Jde
{
	using boost::container::flat_set;
#define RETURN(x) auto pDataSource = DataSource(); return !pDataSource ? x : pDataSource

	template<class T> optional<T> DB::Scaler( sv sql, const vector<DataValue>& parameters )noexcept
	{
		RETURN( optional<T>{} )->Scaler<T>( sql, parameters );
	}
	template<class T> optional<T> DB::TryScaler( sv sql, const vector<DataValue>& parameters )noexcept
	{
		RETURN( optional<T>{} )->TryScaler<T>( sql, parameters );
	}

/*	sp<template<class K,class V>> flat_map<K,V> DB::SelectMap( sv sql )noexcept(false)
	{
		auto p = DataSource();
		return p ? p->SelectMap<K,V>( sql ) : flat_map<K,V>{};
	}
	template<class K,class V> flat_map<K,V> DB::SelectMapSP( sv sql )noexcept(false)
	{
		auto p = DataSource();
		return p ? p->SelectMapSP<K,V>( sql ) : sp<flat_map<K,V>>{};
	}*/
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

#undef var
#undef  🚪
}