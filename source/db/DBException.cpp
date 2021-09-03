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
	DBException::DBException( const std::runtime_error& inner, sv sql, const std::vector<DataValue>* pValues, uint errorCode )noexcept:
		RuntimeException( inner ),
		Sql{sql},
		Parameters{ CopyParams(pValues) },
		ErrorCode{ errorCode }
	{}

	DBException::DBException( uint errorCode, sv sql, const std::vector<DataValue>* pValues )noexcept:
		DBException{ std::runtime_error{""}, sql, pValues, errorCode }
	{}

	DBException::DBException( sv sql, const std::vector<DataValue>* pValues )noexcept:
		DBException{ 0, sql, pValues }
	{}

	const char* DBException::what() const noexcept
	{
		if( Sql.size() && _what.empty() )
			_what =  DB::Message( Sql, &Parameters, string{RuntimeException::what()} );
		return RuntimeException::what();
	}

	void DBException::Log( sv /*pszAdditionalInformation*/, optional<ELogLevel> pLevel )const noexcept
	{
		if( Sql.find("log_message_insert")==string::npos )
			DB::Log( Sql, Parameters.size() ? &Parameters : nullptr, _fileName, _functionName, _line, pLevel ? *pLevel : _level, _pInner ? string{_pInner->what()} : what() );
		else
			ERRX( "log_message_insert sql='{}'"sv, Sql );
	}
}