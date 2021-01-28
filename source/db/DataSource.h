#pragma once
#include "Row.h"
//#include "../../types/Schema.h"
#include "SchemaProc.h"

namespace Jde::DB
{

	namespace Types{ struct IRow; }
	struct IDataSource : std::enable_shared_from_this<IDataSource>
	{
		virtual ~IDataSource() = default;

		virtual sp<ISchemaProc> SchemaProc()noexcept=0;
		virtual uint Scaler( string_view sql, const std::vector<DataValue>& parameters )noexcept(false)=0;
		virtual optional<uint> ScalerOptional( string_view sql, const std::vector<DataValue>& parameters )noexcept(false)=0;
		virtual uint Execute( string_view sql )noexcept(false)=0;
		virtual uint Execute( string_view sql, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)=0;
		virtual uint Execute( string_view sql, const std::vector<DataValue>& parameters, std::function<void(const IRow&)> f, bool log=true )noexcept(false)=0;
		virtual uint ExecuteProc( string_view sql, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)=0;
		virtual uint ExecuteProc( string_view sql, const std::vector<DataValue>& parameters, std::function<void(const IRow&)> f, bool log=true )noexcept(false)=0;

		virtual void Select( string_view sql, std::function<void(const IRow&)> f, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)=0;
		virtual void Select( string_view sql, std::function<void(const IRow&)> f )noexcept(false)=0;
		template<typename K,typename V>
		flat_map<K,V> SelectMap( sv sql )noexcept(false);
		void Log( string_view /*sql*/, const std::vector<DataValue>* /*pParameters*/ )noexcept{};
		virtual string Catalog()noexcept=0;
		string ConnectionString;
	};

	template<>
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
	}

}