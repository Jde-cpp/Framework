#pragma once
#include <jde/Exports.h>
#include "DataType.h"
#include <jde/Exception.h>

#define DB_TRY(x) try{x;}catch( const DB::DBException& ){}
#define THROW_DB(sql,params)
namespace Jde::DB
{
	struct Γ DBException final: IException
	{
		DBException( int32 errorCode, string sql, const vector<object>* pValues, string what, SRCE )noexcept;
		DBException( string sql, const vector<object>* pValues, string what, SRCE )noexcept:DBException{ 0, move(sql), pValues, move(what), sl }{}
		DBException( DBException&& from )noexcept:IException{move(from)}, Sql{from.Sql}, Parameters{from.Parameters}{}
		~DBException()noexcept{ Log(); SetLevel( ELogLevel::NoLog ); };

		α Log()const noexcept->void override;
		α what() const noexcept->const char* override;
		using T=DBException;
		α Clone()noexcept->sp<IException> override{ return ms<T>(move(*this)); }
		α Move()noexcept->up<IException> override{ return mu<T>(move(*this)); }
		α Ptr()->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }

		const string Sql;
		const vector<object> Parameters;
	};
}