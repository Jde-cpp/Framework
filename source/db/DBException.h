#pragma once
#include <jde/Exports.h>
#include "DataType.h"
#include <jde/Exception.h>

#define DB_TRY(x) try{x;}catch( const DB::DBException&){}
#define THROW_DB(sql,params)
namespace Jde::DB
{
	struct JDE_NATIVE_VISIBILITY DBException final: RuntimeException
	{
		DBException( uint errorCode, sv sql, const std::vector<DataValue>* pValues=nullptr )noexcept;
		DBException( sv sql, const std::vector<DataValue>* pValues=nullptr )noexcept;
		DBException( const std::runtime_error& inner, sv sql, const std::vector<DataValue>* pValues=nullptr, uint errorCode=0 )noexcept;

		void Log( sv pszAdditionalInformation="", ELogLevel level=ELogLevel::Debug )const noexcept override;
		template<class... Args>
		DBException( sv value, Args&&... args ):
			RuntimeException( value, args... ),
			ErrorCode{0}
		{
		//	DBG( string{what()} );
		}
		const char* what() const noexcept override;
		const std::string Sql;
		const std::vector<DataValue> Parameters;
		const uint ErrorCode;
	private:
	};
}