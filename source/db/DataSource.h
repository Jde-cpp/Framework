#pragma once
#include "Row.h"
#include "SchemaProc.h"
#include "../coroutine/Awaitable.h"
#include <jde/Log.h>

#define DBLOG(message,params) Jde::DB::Log( message, params, MY_FILE, __func__, __LINE__ )
namespace Jde::DB
{
	using namespace Coroutine;
	namespace Types{ struct IRow; }
	struct JDE_NATIVE_VISIBILITY IDataSource : std::enable_shared_from_this<IDataSource>
	{
		virtual ~IDataSource() = default;

		virtual sp<ISchemaProc> SchemaProc()noexcept=0;
		uint ScalerNonNull( sv sql, const std::vector<DataValue>& parameters )noexcept(false);
		virtual void SetAsynchronous()noexcept(false)=0;

		template<typename T> optional<T> TryScaler( sv sql, const vector<DataValue>& parameters )noexcept;
		template<typename T> optional<T> Scaler( sv sql, const vector<DataValue>& parameters )noexcept(false);
		//virtual ⓣ ScalerCo( string&& sql, const vector<DataValue>&& parameters )noexcept(false){ throw Exception( "Not implemented" ); }

		optional<uint> TryExecute( sv sql )noexcept;
		optional<uint> TryExecute( sv sql, const std::vector<DataValue>& parameters, bool log=true )noexcept;
		optional<uint> TryExecuteProc( sv sql, const std::vector<DataValue>& parameters, bool log=true )noexcept;

		virtual uint Execute( sv sql )noexcept(false)=0;
		virtual uint Execute( sv sql, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)=0;
		virtual uint Execute( sv sql, const std::vector<DataValue>* pParameters, std::function<void(const IRow&)>* f, bool isStoredProc=false, bool log=true )noexcept(false)=0;
		virtual uint ExecuteProc( sv sql, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)=0;
		virtual uint ExecuteProc( sv sql, const std::vector<DataValue>& parameters, std::function<void(const IRow&)> f, bool log=true )noexcept(false)=0;

		void Select( sv sql, std::function<void(const IRow&)> f, const std::vector<DataValue>& parameters, bool log=true )noexcept(false);
		void Select( sv sql, std::function<void(const IRow&)> f )noexcept(false);
		virtual uint Select( sv sql, std::function<void(const IRow&)> f, const vector<DataValue>* pValues, bool log )noexcept(false)=0;
		virtual AsyncAwaitable SelectCo( string&& sql, std::function<void(const IRow&)> f, const std::vector<DataValue>&& parameters, bool log )noexcept=0;//[[noreturn]]{ throw Exception("Not implemented"); }//return FunctionAwaitable{ []( coroutine_handle<Task2::promise_type> ){} }; }//
		bool TrySelect( sv sql, std::function<void(const IRow&)> f )noexcept;
		template<class K,class V> sp<flat_map<K,V>> SelectMap( sv sql )noexcept(false);
		string Catalog( sv sql )noexcept(false);
		bool Asynchronous{false};
		const string& ConnectionString()const noexcept{ return _connectionString; }
		virtual void SetConnectionString( sv x )noexcept{ _connectionString = x; }
	protected:
		string _connectionString;
	};

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