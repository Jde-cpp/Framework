#include "DBException.h"
#include "Database.h"

namespace Jde::DB
{
	α CopyParams( const std::vector<DataValue>* pValues )->const std::vector<DataValue>
	{
		std::vector<DataValue> y;
		if( pValues )
		{
			for( uint i=0; i<pValues->size(); ++i )
				y.push_back( EDataValue::StringView==(EDataValue)(*pValues)[i].index() ? DataValue{ string{ get<sv>((*pValues)[i])} } : (*pValues)[i] );
		}
		return y;
	}
	DBException::DBException( std::runtime_error&& e, sv sql, const vector<DataValue>* pValues, const source_location& sl )noexcept:
		IException{ sl, move(e) },
		Sql{ sql },
		Parameters{ CopyParams(pValues) }
	{
		Log();
	}

	DBException::DBException( _int errorCode, sv sql, const std::vector<DataValue>* pValues, str what, const source_location& sl )noexcept:
		IException{ what, sl },
		Sql{ sql },
		Parameters{ CopyParams(pValues) },
		ErrorCode{ errorCode }
	{}

	α DBException::what()const noexcept->const char*
	{
		if( Sql.size() && _what.empty() )
			_what =  DB::Message( Sql, &Parameters, string{IException::what()} );
		return IException::what();
	}

	α DBException::Log()const noexcept->void
	{
		if( Sql.find("log_message_insert")==string::npos )
			DB::Log( Sql, Parameters.size() ? &Parameters : nullptr, _level, _pInner ? string{_pInner->what()} : what(), _sl );
		else
			ERRX( "log_message_insert sql='{}'"sv, Sql );
	}
}