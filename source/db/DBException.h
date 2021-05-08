#pragma once
#include "../Exports.h"
#include "DataType.h"
//#include "../log/Logging.h"
#include "../Exception.h"

#define DB_TRY(x) try{x;}catch( const DB::DBException&){}
#define THROW_DB(sql,params)
namespace Jde::DB
{
	struct JDE_NATIVE_VISIBILITY DBException final: RuntimeException
	{
		DBException( const std::runtime_error& inner, sv sql, const std::vector<DataValue>* pValues=nullptr )noexcept;

		void Log( sv pszAdditionalInformation="", ELogLevel level=ELogLevel::Debug )const noexcept;
		template<class... Args>
		DBException( sv value, Args&&... args ):
			RuntimeException( value, args... )
		{
		//	DBG( string{what()} );
		}

		const std::string Sql;
		const std::vector<DataValue> Parameters;
	private:
	};
}