#include "DBException.h"
#include "Database.h"

namespace Jde::DB
{
	α CopyParams( const std::vector<object>* pValues )->const std::vector<object>
	{
		std::vector<object> y;
		if( pValues )
		{
			for( uint i=0; i<pValues->size(); ++i )
				y.push_back( EObject::StringView==(EObject)(*pValues)[i].index() ? object{ string{ get<sv>((*pValues)[i])} } : (*pValues)[i] );
		}
		return y;
	}

	DBException::DBException( _int errorCode, sv sql, const std::vector<object>* pValues, str what, const source_location& sl )noexcept:
		IException{ what, sl },
		Sql{ sql },
		Parameters{ CopyParams(pValues) },
		ErrorCode{ errorCode }
	{
		Log();
	}

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