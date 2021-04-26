#include "DBException.h"
#include "Database.h"

namespace Jde::DB
{
	const std::vector<DataValue> CopyParams( const std::vector<DataValue>* pValues )
	{
		std::vector<DataValue> y;
		if( pValues )
		{
			for( uint i=0; i<pValues->size(); ++i )
				y.push_back( EDataValue::StringView==(EDataValue)(*pValues)[i].index() ? DataValue{ string{ get<sv>((*pValues)[i])} } : (*pValues)[i] );
		}
		return y;
	}
	DBException::DBException( const std::runtime_error& inner, sv sql, const std::vector<DataValue>* pValues )noexcept:
		RuntimeException( inner ),
		Sql{sql},
		Parameters{ CopyParams(pValues) }
	{}

	void DBException::Log( sv /*pszAdditionalInformation*/, ELogLevel level )const noexcept
	{
		if( Sql.find("log_message_insert")==string::npos )
			DB::Log( Sql, Parameters.size() ? &Parameters : nullptr, _fileName, _functionName, _line, level, _pInner ? string{_pInner->what()} : string{} );
		else
			ERRX( "log_message_insert sql='{}'"sv, Sql );
	}
}