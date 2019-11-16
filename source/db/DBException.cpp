#include "stdafx.h"
#include "DBException.h"

namespace Jde::DB
{
	DBException::DBException( const std::runtime_error& inner, string_view sql, const std::vector<DataValue>* pValues ):
		RuntimeException( inner ),
		Sql{sql},
		Parameters{ pValues ? *pValues : std::vector<DataValue>{} }
	{
		if( sql.find("log_message_insert")==string::npos )
			DBG0( sql );
	}
}