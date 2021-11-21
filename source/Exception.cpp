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

	IException::IException( sv what, const source_location& sl )noexcept:
		_sl{ sl },
		_what{ what }
	{}

	IException::~IException()
	{}

	α IException::Log()const noexcept->void
	{
		if( _level==ELogLevel::NoLog ) return;
		const string fileName{ strlen(_sl.file_name()) ? FileName(_sl.file_name()) : "{Unknown}\0"sv };
		var functionName = strlen(_sl.file_name()) ? _sl.function_name() : "{Unknown}\0";
		Logging::Default().log( spdlog::source_loc{fileName.c_str(), (int)_sl.line(), functionName}, (spdlog::level::level_enum)_level, what() );
		if( Logging::Server() )
			Logging::LogServer( Logging::Messages::ServerMessage{Logging::Message{_level, what(), _sl}, vector<string>{_args}} );
	}

	α IException::what()const noexcept->const char*
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

	α CodeException::ToString( const std::error_code& errorCode )noexcept->string
	{
		var value = errorCode.value();
		var& category = errorCode.category();
		var message = errorCode.message();
		return format( "({}){} - {})", value, category.name(), message );
	}

	α CodeException::ToString( const std::error_category& errorCategory )noexcept->string{	return errorCategory.name(); }

	α CodeException::ToString( const std::error_condition& errorCondition )noexcept->string
	{
		const int value = errorCondition.value();
		const std::error_category& category = errorCondition.category();
		const string message = errorCondition.message();
		return format( "({}){} - {})", value, category.name(), message );
	}

	OSException::OSException( T result, string&& msg, const source_location& sl )noexcept:
#ifdef _MSC_VER
		IException{ {std::to_string(result), std::to_string(GetLastError()), move(msg)}, "result={}/error={} - {}", sl }
#else
		IException{ {std::to_string(result), std::to_string(errno), move(msg)}, "result={}/error={} - {}", sl }
#endif
	{
		Log();
	}

	Exception::Exception( sv what, ELogLevel l, const source_location& sl )noexcept:
		IException( l, what, sl )
	{}

	α IOException::ErrorCode()const noexcept->uint
	{
		return  _pUnderLying ? _pUnderLying->code().value() : _errorCode;
	}

	α IOException::Path()const noexcept->path
	{
		return  _pUnderLying? _pUnderLying->path1() : _path;
	}

	α IOException::what()const noexcept->const char*
	{
		_what = _pUnderLying ? _pUnderLying->what() : ErrorCode()
			? format( "({}) {} - {} path='{}'", ErrorCode(), std::strerror(errno), IException::what(), Path().string() )
			: format( "'{}' - {}", IException::what(), Path().string() );
		return _what.c_str();
	}

	// α IOException::Log()const noexcept->void
	// {
	// }
}