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
		DBException( _int errorCode, string sql, const vector<object>* pValues, string what, SRCE )noexcept;
		DBException( string sql, const vector<object>* pValues, string what, SRCE )noexcept:DBException{ 0, move(sql), pValues, move(what), sl }{};
		α Clone()noexcept->sp<IException> override{ return std::make_shared<DBException>(move(*this)); }
		α Log()const noexcept->void override;
		α what() const noexcept->const char* override;
		α Ptr()->std::exception_ptr override{ return std::make_exception_ptr(*this); }
		[[noreturn]] α Throw()->void override{ throw *this; }

		const string Sql;
		const vector<object> Parameters;
		const _int ErrorCode{ 0 };
	};
}