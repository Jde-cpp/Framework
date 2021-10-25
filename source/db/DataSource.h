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
	struct Γ IDataSource : std::enable_shared_from_this<IDataSource>
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

		β Execute( sv sql )noexcept(false)->uint=0;
		β Execute( sv sql, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)->uint = 0;
		β Execute( sv sql, const std::vector<DataValue>* pParameters, std::function<void(const IRow&)>* f, bool isStoredProc=false, bool log=true )noexcept(false)->uint = 0;
		β ExecuteProc( sv sql, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)->uint = 0;
		β ExecuteProc( sv sql, const std::vector<DataValue>& parameters, std::function<void(const IRow&)> f, bool log=true )noexcept(false)->uint = 0;

		α Select( sv sql, std::function<void(const IRow&)> f, const std::vector<DataValue>& parameters, bool log=true, SRCE )noexcept(false)->void;
		α Select( sv sql, std::function<void(const IRow&)> f )noexcept(false)->void;
		β Select( sv sql, std::function<void(const IRow&)> f, const vector<DataValue>* pValues, bool log, SRCE )noexcept(false)->uint=0;
		β SelectCo( string&& sql, std::function<void(const IRow&)> f, const std::vector<DataValue>&& parameters, bool log )noexcept->up<IAwaitable> = 0;//[[noreturn]]{ throw Exception("Not implemented"); }//return FunctionAwaitable{ []( coroutine_handle<Task2::promise_type> ){} }; }//
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