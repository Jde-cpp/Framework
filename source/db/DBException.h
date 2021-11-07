#pragma once
#include <jde/Exports.h>
#include "DataType.h"
#include <jde/Exception.h>

#define DB_TRY(x) try{x;}catch( const DB::DBException&){}
#define THROW_DB(sql,params)
namespace Jde::DB
{
	struct Γ DBException final: IException
	{
		DBException( std::runtime_error&& e, sv sql, const vector<DataValue>* pValues=nullptr, SRCE )noexcept;
		//DBException( _int errorCode, sv sql, const vector<DataValue>* pValues=nullptr, SRCE )noexcept;
//		DBException( sv sql, const vector<DataValue>* pValues=nullptr, SRCE )noexcept;
		DBException( _int errorCode, sv sql, const vector<DataValue>* pValues, str what, SRCE )noexcept;

/*		template<class... Args>
		DBException( sv value, Args&&... args ):
			IException( value, args... ),
			ErrorCode{999}
		{
			_level = ELogLevel::Error;
		}
*/
		α Log()const noexcept->void override;
		α what() const noexcept->const char* override;
		const string Sql;
		const vector<DataValue> Parameters;
		const _int ErrorCode{0};
	};
}