#pragma once
#include "../Exports.h"
#include "DataType.h"

#define DB_TRY(x) try{x;}catch( const DB::DBException&){}
namespace Jde::DB
{
	struct JDE_NATIVE_VISIBILITY DBException : RuntimeException
	{
		DBException( const std::runtime_error& inner, string_view sql, const std::vector<DataValue>* pValues=nullptr );
		template<class... Args>
		DBException( std::string_view value, Args&&... args ):
			RuntimeException( value, args... )
		{
			DBG0( what() );
		}


		const std::string Sql;
		const std::vector<DataValue> Parameters;
	private:
	};
}