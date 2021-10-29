#include <jde/Exception.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <boost/system/error_code.hpp>

#include <jde/Log.h>
#include "log/server/ServerSink.h"
#include <jde/TypeDefs.h>
#define var const auto

namespace Jde
{
/*	void catch_exception( sv pszFunction, sv pszFile, long line, sv pszAdditional, const std::exception* pException )
	{
		WARN( "{} - ({}){}({}) - {}", pException ? pException->what() : "Unknown", pszFunction, pszFile, line, pszAdditional );
	}
*/
	IException::IException( ELogLevel level, sv value, const source_location& sl )noexcept:
		_sl{ sl },
		_level{level},
		_what{value}
	{}
	IException::IException( vector<string>&& args, string&& format, const source_location& sl, ELogLevel level )noexcept:
		_sl{ sl },
		_level{level},
		_format{ move(format) },
		_args{ move(args) }
	{}

	/*
	IException::IException( ELogLevel level, sv value )noexcept:
		_level{ level },
		_what{ value }
	{
		Log();
	}
	*/
	IException::IException( sv what, const source_location& sl )noexcept:
		_sl{ sl },
		_what{ what }
	{}

/*	IException::IException( std::exception&& e, const source_location& sl )noexcept:
		base{ move(e) },
		_sl{ sl }
	{}
*/
/*	IException::IException( string&& exp, const source_location& sl )noexcept:
		_what{ move(exp) },
		_functionName{ sl.function_name() },
		_fileName{ sl.file_name() },
		_line{ sl.line() }
	{}
*/
	IException::~IException()
	{
		Log();
	}

	α IException::Log()const noexcept->void
	{
		if( _level==ELogLevel::NoLog ) return;
		const string fileName{ strlen(_sl.file_name()) ? FileName(_sl.file_name()) : "{Unknown}\0"sv };
		var functionName = strlen(_sl.file_name()) ? _sl.function_name() : "{Unknown}\0";
		Logging::Default().log( spdlog::source_loc{fileName.c_str(), (int)_sl.line(), functionName}, (spdlog::level::level_enum)_level, what() );
		if( Logging::Server() )
			Logging::LogServer( Logging::Messages::ServerMessage{Logging::Message{_level, what(), _sl}, vector<string>{_args}} );
	}

	const char* IException::what()const noexcept
	{
		if( _what.empty() )
		{
			using ctx = fmt::format_context;
			vector<fmt::basic_format_arg<ctx>> args2;
			for( var& a : _args )
				args2.push_back( fmt::detail::make_arg<ctx>(a) );
			_what = fmt::vformat( _format, fmt::basic_format_args<ctx>(args2.data(), (int)args2.size()) );
		}
		return _what.c_str();
	}

	CodeException::CodeException( sv value, std::error_code&& code, ELogLevel level, const source_location& sl ):
		IException{ value, sl },
		_errorCode{ move(code) }
	{
		_level = level;
	}

	CodeException::CodeException( std::error_code&& code, ELogLevel level, const source_location& sl ):
		CodeException( format("{}-{}", code.value(), code.message()), move(code), level, sl )
	{}
/*	α CodeException::Log()const noexcept->void
	{
		var what = format( "{} - {}", _what, ToString(_errorCode) );
		Logging::Default().log( spdlog::source_loc{FileName(_sl.file_name()).c_str(),(int)_sl.line(),_functionName.data()}, (spdlog::level::level_enum)_level, what );
		if( Logging::Server() )
			Logging::LogServer( Logging::Messages::ServerMessage{Logging::Message{_level, what, _fileName, _functionName, _sl.line()}, vector<string>{_args}} );
	}
*/
	BoostCodeException::BoostCodeException( const boost::system::error_code& errorCode, sv msg, const source_location& sl )noexcept:
		IException{ string{msg}, sl },
		_errorCode{ make_unique<boost::system::error_code>(errorCode) }
	{}
	BoostCodeException::BoostCodeException( const BoostCodeException& e, const source_location& sl )noexcept:
		IException{ {}, sl },
		_errorCode{ make_unique<boost::system::error_code>(*e._errorCode) }
	{}
	BoostCodeException::~BoostCodeException()
	{}

/*	α BoostCodeException::Log()const noexcept->void
	{
		Logging::Default().log( spdlog::source_loc{FileName(_fileName).c_str(),(int)_sl.line(),_functionName.data()}, (spdlog::level::level_enum)_level, _what );
	}
*/
	string CodeException::ToString( const std::error_code& errorCode )noexcept
	{
		var value = errorCode.value();
		var& category = errorCode.category();
		var message = errorCode.message();
		return format( "({}){} - {})", value, category.name(), message );
	}
	string CodeException::ToString( const std::error_category& errorCategory )noexcept
	{
		return errorCategory.name();
	}
	string CodeException::ToString( const std::error_condition& errorCondition )noexcept
	{
		const int value = errorCondition.value();
		const std::error_category& category = errorCondition.category();
		const string message = errorCondition.message();
		return format( "({}){} - {})", value, category.name(), message );
	}

	OSException::OSException( T result, string&& msg, const source_location& sl )noexcept:
#ifdef _MSC_VER
		IException{ {std::to_string(result), std::to_string(GetLastError()), move(msg)}, "result={}/error={} - {}", sl }
		//_error{ GetLastError() },
#else
		IException{ {std::to_string(result), std::to_string(errno), move(msg)}, "result={}/error={} - {}", sl }
		//_error{ errno },
#endif
		//_result{ result }
	{
		Log();
	}
/*	α OSException::Log()const noexcept->void
	{
		if( _level==ELogLevel::NoLog ) return;

		using ctx = fmt::format_context;
		vector<fmt::basic_format_arg<ctx>> args;
		for( var& a : _args )
			args.push_back( fmt::detail::make_arg<ctx>(a) );

		Logging::Default().log( spdlog::source_loc{FileName(_fileName).c_str(),(int)_sl.line(),_functionName.data()}, (spdlog::level::level_enum)_level, fmt::vformat(_format, fmt::basic_format_args<ctx>(args.data(), (int)args.size())) );
		if( Logging::Server() )
			Logging::LogServer( Logging::Messages::ServerMessage{Logging::Message{_level, _format, _fileName, _functionName, _sl.line()}, vector<string>{_args}} );
	}
*/
	/*
	Exception::Exception( const std::runtime_error& inner ):
		Exception( inner )
	{}
	*/
	Exception::Exception( sv what, ELogLevel l, const source_location& sl )noexcept:
		IException( l, what, sl )
	{}


	uint IOException::ErrorCode()const noexcept
	{
		return  _pUnderLying ? _pUnderLying->code().value() : _errorCode;
	}
	path IOException::Path()const noexcept
	{
		return  _pUnderLying? _pUnderLying->path1() : _path;
	}

	const char* IOException::what()const noexcept
	{
		_what = _pUnderLying ? _pUnderLying->what() : format( "({}) {} - {} path='{}'", ErrorCode(), std::strerror(errno), IException::what(), Path().string() );
		return _what.c_str();
	}

	α IOException::Log()const noexcept->void
	{

		_what = _pUnderLying ? _pUnderLying->what() : format( "({}) {} - {} path='{}'", ErrorCode(), std::strerror(errno), IException::what(), Path().string() );
		//return _what.c_str();
	}
}