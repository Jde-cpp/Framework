#include "DBException.h"
#include "Database.h"

namespace Jde::DB
{
	DBException::DBException( const std::runtime_error& inner, sv sql, const std::vector<DataValue>* pValues )noexcept:
		RuntimeException( inner ),
		Sql{sql},
		Parameters{ pValues ? *pValues : std::vector<DataValue>{} }
	{}

	void DBException::Log( sv /*pszAdditionalInformation*/, ELogLevel level )const noexcept
	{
		if( Sql.find("log_message_insert")==string::npos )
			DB::Log( Sql, Parameters.size() ? &Parameters : nullptr, _fileName, _functionName, _line, level );
		else
			ERRX( "log_message_insert sql='{}'"sv, Sql );
	}
}