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
	DBException::DBException( _int errorCode, sv sql, const vector<DataValue>* pValues, string&& what, const source_location& sl )noexcept:
		IException{ move(what), sl },
		Sql{ sql },
		Parameters{ CopyParams(pValues) },
		ErrorCode{ errorCode }
	{
		Log();
	}

/*	DBException::DBException( _int errorCode, sv sql, const std::vector<DataValue>* pValues, const source_location& sl )noexcept:
		DBException{ std::runtime_error{""}, sql, pValues, errorCode, sl }
	{}
	
	DBException::DBException( _int errorCode, sv sql, const vector<DataValue>* pValues, str what, const source_location& sl )noexcept:
		DBException{ std::runtime_error{what}, sql, pValues, errorCode, sl }
	{}

	DBException::DBException( sv sql, const std::vector<DataValue>* pValues, const source_location& sl )noexcept:
		DBException{ 0, sql, pValues, sl }
	{}
	*/
	const char* DBException::what() const noexcept
	{
		if( Sql.size() && _what.empty() )
			_what =  DB::Message( Sql, &Parameters, string{IException::what()} );
		return IException::what();
	}

	void DBException::Log()const noexcept
	{
		if( Sql.find("log_message_insert")==string::npos )
			DB::Log( Sql, Parameters.size() ? &Parameters : nullptr, _fileName, _functionName, _line, _level, _pInner ? string{_pInner->what()} : what() );
		else
			ERRX( "log_message_insert sql='{}'"sv, Sql );
	}
}