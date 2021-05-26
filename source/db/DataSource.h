#pragma once
#include "Row.h"
//#include "../../types/Schema.h"
#include "SchemaProc.h"
#include <jde/Log.h>

#define DBLOG(message,params) Jde::DB::Log( message, params, MY_FILE, __func__, __LINE__ )
namespace Jde::DB
{
	namespace Types{ struct IRow; }
	struct JDE_NATIVE_VISIBILITY IDataSource : std::enable_shared_from_this<IDataSource>
	{
		virtual ~IDataSource() = default;

		virtual sp<ISchemaProc> SchemaProc()noexcept=0;
		uint ScalerNonNull( sv sql, const std::vector<DataValue>& parameters )noexcept(false);

		template<typename T> optional<T> TryScaler( sv sql, const vector<DataValue>& parameters )noexcept;
		template<typename T> optional<T> Scaler( sv sql, const vector<DataValue>& parameters )noexcept(false);

		//optional<uint> ScalerOptional( sv sql, const std::vector<DataValue>& parameters )noexcept(false);

		optional<uint> TryExecute( sv sql )noexcept;
		optional<uint> TryExecute( sv sql, const std::vector<DataValue>& parameters, bool log=true )noexcept;
		optional<uint> TryExecuteProc( sv sql, const std::vector<DataValue>& parameters, bool log=true )noexcept;

		//virtual uint Execute( sv sql, const std::vector<DataValue>& parameters, std::function<void(const IRow&)> f, bool log=true )noexcept(false)=0;
		virtual uint Execute( sv sql )noexcept(false)=0;
		virtual uint Execute( sv sql, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)=0;
		virtual uint Execute( sv sql, const std::vector<DataValue>* pParameters, std::function<void(const IRow&)>* f, bool isStoredProc=false, bool log=true )noexcept(false)=0;
		virtual uint ExecuteProc( sv sql, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)=0;
		virtual uint ExecuteProc( sv sql, const std::vector<DataValue>& parameters, std::function<void(const IRow&)> f, bool log=true )noexcept(false)=0;

		void Select( sv sql, std::function<void(const IRow&)> f, const std::vector<DataValue>& parameters, bool log=true )noexcept(false);
		void Select( sv sql, std::function<void(const IRow&)> f )noexcept(false);
		virtual uint Select( sv sql, std::function<void(const IRow&)> f, const vector<DataValue>* pValues, bool log )noexcept(false)=0;
		bool TrySelect( sv sql, std::function<void(const IRow&)> f )noexcept;
		//template<class K,class V> void SelectMap( sv sql, flat_map<K,V>& out )noexcept(false);
		//template<class K,class V> flat_map<K,V> SelectMap( sv sql )noexcept(false);
		template<class K,class V> sp<flat_map<K,V>> SelectMap( sv sql )noexcept(false);
		string Catalog( sv sql )noexcept(false);
	//protected:
		string ConnectionString;
	};

/*	template<>
	inline flat_map<uint,string> IDataSource::SelectMap<uint,string>( sv sql )noexcept(false)
	{
		flat_map<uint,string> results;
		auto fnctn = [&]( const IRow& row ){ results.emplace(row.GetUInt(0), row.GetString(1)); };
		Select( sql, fnctn );
		return results;
	}

	template<>
	inline flat_map<string,uint> IDataSource::SelectMap<string,uint>( sv sql )noexcept(false)
	{
		flat_map<string,uint> results;
		auto fnctn = [&]( const IRow& row ){ results.emplace(row.GetString(0), row.GetUInt(1)); };
		Select( sql, fnctn );
		return results;
	}*/

	template<typename T>
	optional<T> IDataSource::TryScaler( sv sql, const vector<DataValue>& parameters )noexcept
	{
		optional<T> result;
		Try( [&]{ Select( sql, [&result](const IRow& row){ result = row.Get<T>(0); }, parameters);} );
		return result;
	}
	template<typename T> optional<T> IDataSource::Scaler( sv sql, const vector<DataValue>& parameters )noexcept(false)
	{
		optional<T> result;
		Select( sql, [&result](const IRow& row){ result = row.Get<T>(0); }, parameters );
		return result;
	}

	template<class K,class V> sp<flat_map<K,V>> IDataSource::SelectMap( sv sql )noexcept(false)
	{
		auto y = make_shared<flat_map<K,V>>();
		Select( sql, [&y]( const IRow& row ){y->emplace(row.Get<K>(0), row.Get<V>(1));} );
		return y;
	}
}