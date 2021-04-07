#pragma once
#include "Row.h"
//#include "../../types/Schema.h"
#include "SchemaProc.h"
#include "../log/Logging.h"

#define DBLOG(message,params) Jde::DB::IDataSource::Log2( message, params, MY_FILE, __func__, __LINE__ )
namespace Jde::DB
{
	namespace Types{ struct IRow; }
	struct JDE_NATIVE_VISIBILITY IDataSource : std::enable_shared_from_this<IDataSource>
	{
		virtual ~IDataSource() = default;

		virtual sp<ISchemaProc> SchemaProc()noexcept=0;
		uint Scaler( sv sql, const std::vector<DataValue>& parameters )noexcept(false);

		template<typename T>
		optional<T> TryScaler( sv sql, const vector<DataValue>& parameters )noexcept;

		optional<uint> ScalerOptional( sv sql, const std::vector<DataValue>& parameters )noexcept(false);

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
		template<typename K,typename V>
		flat_map<K,V> SelectMap( sv sql )noexcept(false);
		void Log2( sv sql, const std::vector<DataValue>* pParameters, sv file, sv fnctn, uint line )noexcept
		{
			const auto size = pParameters ? pParameters->size() : 0;
			ostringstream os;
			uint prevIndex=0;
			for( uint sqlIndex=0, paramIndex=0; (sqlIndex=sql.find_first_of('?', prevIndex))!=string::npos && paramIndex<size; ++paramIndex, prevIndex=sqlIndex+1 )
			{
				os << sql.substr( prevIndex, sqlIndex-prevIndex );
				os << DB::to_string( (*pParameters)[paramIndex] );
			}
			if( prevIndex<sql.size() )
				os << sql.substr( prevIndex );

			//if( os.str()=="{{error}}" )
				//DBG("HERE"sv);
			Logging::Log( Logging::MessageBase(ELogLevel::Debug, os.str(), file, fnctn, line) );
			//DBG( os.str() );
		};
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

	template<typename K,typename V>
	flat_map<K,V> IDataSource::SelectMap( sv sql )noexcept(false)
	{
		flat_map<K,V> results;
		auto fnctn = [&]( const IRow& row ){ results.emplace(row.Get<K>(0), row.Get<V>(1)); };
		Select( sql, fnctn );
		return results;
	}
}