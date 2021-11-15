#pragma once
#include "Row.h"
#include "SchemaProc.h"
#include "../coroutine/Awaitable.h"
#include <jde/Log.h>

#define DBLOG(sql,params) Jde::DB::Log( sql, params, sl )
namespace Jde::DB
{
	using namespace Coroutine;
	namespace Types{ struct IRow; }
	struct Γ IDataSource : std::enable_shared_from_this<IDataSource>
	{
		virtual ~IDataSource() = default;

		β SchemaProc()noexcept->sp<ISchemaProc> =0;
		α ScalerNonNull( sv sql, const std::vector<DataValue>& parameters )noexcept(false)->uint;
		//virtual void SetAsynchronous()noexcept(false)=0;

		ⓣ TryScaler( sv sql, const vector<DataValue>& parameters )noexcept->optional<T>;
		ⓣ Scaler( sv sql, const vector<DataValue>& parameters )noexcept(false)->optional<T>;
		//virtual ⓣ ScalerCo( string&& sql, const vector<DataValue>&& parameters )noexcept(false){ throw Exception( "Not implemented" ); }

		α TryExecute( sv sql )noexcept->optional<uint>;
		α TryExecute( sv sql, const std::vector<DataValue>& parameters, bool log=true )noexcept->optional<uint>;
		α TryExecuteProc( sv sql, const std::vector<DataValue>& parameters, bool log=true, SRCE )noexcept->optional<uint>;

		β Execute( sv sql, SRCE )noexcept(false)->uint=0;
		β Execute( sv sql, const std::vector<DataValue>& parameters, bool log=true, SRCE )noexcept(false)->uint = 0;
		β Execute( sv sql, const std::vector<DataValue>* pParameters, std::function<void(const IRow&)>* f, bool isStoredProc=false, bool log=true, SRCE )noexcept(false)->uint = 0;
		β ExecuteProc( sv sql, const std::vector<DataValue>& parameters, bool log=true, SRCE )noexcept(false)->uint=0;
		β ExecuteProc( sv sql, const std::vector<DataValue>& parameters, std::function<void(const IRow&)> f, bool log=true, SRCE )noexcept(false)->uint=0;

		α Select( sv sql, std::function<void(const IRow&)> f, const std::vector<DataValue>& parameters, bool log=true, SRCE )noexcept(false)->void;
		α Select( sv sql, std::function<void(const IRow&)> f )noexcept(false)->void;
		β Select( sv sql, std::function<void(const IRow&)> f, const vector<DataValue>* pValues, bool log, SRCE )noexcept(false)->uint=0;
		β SelectCo( string&& sql, std::function<void(const IRow&)> f, const std::vector<DataValue>&& parameters, bool log, SRCE )noexcept->up<IAwaitable> = 0;//[[noreturn]]{ throw Exception("Not implemented"); }//return FunctionAwaitable{ []( coroutine_handle<Task2::promise_type> ){} }; }//
		α TrySelect( sv sql, std::function<void(const IRow&)> f )noexcept->bool;
		ẗ SelectMap( sv sql )noexcept(false)->sp<flat_map<K,V>>;
		α Catalog( sv sql )noexcept(false)->string;

		α ConnectionString()const noexcept->const string&{ return _connectionString; }
		β SetConnectionString( sv x )noexcept->void{ _connectionString = x; }

		//bool Asynchronous{false};

	protected:
		string _connectionString;
	};

	ⓣ IDataSource::TryScaler( sv sql, const vector<DataValue>& parameters )noexcept->optional<T>
	{
		optional<T> result;
		Try( [&]{ Select( sql, [&result](const IRow& row){ result = row.Get<T>(0); }, parameters);} );
		return result;
	}

	ⓣ IDataSource::Scaler( sv sql, const vector<DataValue>& parameters )noexcept(false)->optional<T>
	{
		optional<T> result;
		Select( sql, [&result](const IRow& row){ result = row.Get<T>(0); }, parameters );
		return result;
	}

	ẗ IDataSource::SelectMap( sv sql )noexcept(false)->sp<flat_map<K,V>>
	{
		auto y = make_shared<flat_map<K,V>>();
		Select( sql, [&y]( const IRow& row ){y->emplace(row.Get<K>(0), row.Get<V>(1));} );
		return y;
	}
}