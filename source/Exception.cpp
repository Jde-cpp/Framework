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
	void catch_exception( sv pszFunction, sv pszFile, long line, sv pszAdditional, const std::exception* pException )
	{
		GetDefaultLogger()->warn( "{} - ({}){}({}) - {}", pException ? pException->what() : "Unknown", pszFunction, pszFile, line, pszAdditional );
	}

	Exception::Exception( ELogLevel level, sv value, sv function, sv file, long line )noexcept:
		std::exception(),
		_functionName{function},
		_fileName{file},
		_line{line},
		_level{level},
		_what{value},
		_format{ value }
	{}

	Exception::Exception( ELogLevel level, sv value )noexcept:
		std::exception(),
		_level{ level },
		_what{ value },
		_format{ value }
	{}
	Exception::Exception( sv value )noexcept:
		_what{ value },
		_format{ value }
	{}

	Exception::Exception( const std::exception& exp )noexcept:
		_what{ exp.what() },
		_format( _what )
	{}

	Exception::~Exception()
	{}

	void Exception::Log( sv additionalInformation, ELogLevel level )const noexcept
	{
		std::ostringstream os;
		if( additionalInformation.size() )
			os << "[" << additionalInformation << "] ";
		os << what();
		if( HaveLogger() )
		{
			GetDefaultLogger()->log( (spdlog::level::level_enum)level, os.str() );
			if( GetServerSink() )
				LogServer( Logging::Messages::Message{level, os.str(), _fileName, _functionName, (uint32)_line, _args} );
		}
		else
			std::cerr << os.str() << endl;
	}
	std::ostream& operator<<( std::ostream& os, const Exception& e )
	{
		os << e.what() << " - (" << e._functionName << ")" << e._fileName << "(" << e._line << ")";
		return os;
	}

	const char* Exception::what()const noexcept
	{
		return _what.c_str();
	}

	CodeException::CodeException( sv value, const std::error_code& code, ELogLevel level ):
		RuntimeException{ value },
		_pErrorCode{ std::make_shared<std::error_code>(code) }
	{
		_level = level;
	}

	BoostCodeException::BoostCodeException( const boost::system::error_code& errorCode, sv msg )noexcept:
		RuntimeException{ string{msg} },
		_errorCode{ make_unique<boost::system::error_code>(errorCode) }
	{}
	BoostCodeException::BoostCodeException( const BoostCodeException& e )noexcept:
		_errorCode{ make_unique<boost::system::error_code>(*e._errorCode) }
	{}
	BoostCodeException::~BoostCodeException()
	{}


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

	RuntimeException::RuntimeException( const std::runtime_error& inner ):
		Exception( inner )
	{}


	uint IOException::ErrorCode()const noexcept
	{
		return  _pUnderLying ? _pUnderLying->code().value() : _errorCode;
	}
	void IOException::TestExists( path path )noexcept(false)
	{
		THROW_IF( !fs::exists(path), IOException{path, "does not exist"} );
	}
	path IOException::Path()const noexcept
	{
		return  _pUnderLying? _pUnderLying->path1() : _path;
	}

	const char* IOException::what()const noexcept
	{
		_what = _pUnderLying ? _pUnderLying->what() : format( "({}) {} - {} path='{}'", ErrorCode(), std::strerror(errno), RuntimeException::what(), Path().string() );
		return _what.c_str();
	}

}