#include "Exception.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include "log/Logging.h"
#include "log/server/ServerSink.h"
#include "TypeDefs.h"
#define var const auto

namespace Jde
{
	void catch_exception( std::string_view pszFunction, std::string_view pszFile, long line, std::string_view pszAdditional, const std::exception* pException )
	{
		GetDefaultLogger()->warn( "{} - ({}){}({}) - {}", pException ? pException->what() : "Unknown", pszFunction, pszFile, line, pszAdditional );
	}

	Exception::Exception( ELogLevel level, std::string_view value ):
		std::exception(),
		_level{level},
		_format{ value },
		_what(value)
	{}
	Exception::Exception( std::string_view value ):
		_format{ value },
		_what(value)
	{}

	Exception::Exception( const std::exception& exp ):
		_format( exp.what() ),
		_what( _format )
	{}

	Exception::~Exception()
	{
		//std::cerr << "here";
	}

	void Exception::Log( std::string_view pszAdditionalInformation, ELogLevel level )const
	{
		std::ostringstream os; std::ostringstream os2;
		if( pszAdditionalInformation.size() )
		{
			os << "[" << pszAdditionalInformation << "] ";
			os2 << os.str();
		}
		os << _what;
		os2 << _format;
		if( HaveLogger() )
		{
			GetDefaultLogger()->log( (spdlog::level::level_enum)level, os.str() );
			if( GetServerSink() )
				LogServer( Logging::Messages::Message{level, os2.str(), _fileName, _functionName, (uint32)_line, _args} );
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

	CodeException::CodeException( std::string_view value, const std::error_code& code, ELogLevel level ):
		RuntimeException{ value },
		_pErrorCode{ std::make_shared<std::error_code>(code) }
	{
		_level = level;
	}

	BoostCodeException::BoostCodeException( const boost::system::error_code& errorCode ):
		_errorCode{ errorCode }
	{}

/*	EnvironmentException::EnvironmentException( std::string_view value ):
		Exception( value, ELogLevel::Critical )
	{}
*/
	string CodeException::ToString( const std::error_code& errorCode )noexcept
	{
		const int value = errorCode.value();
		const std::error_category& category = errorCode.category();
		//const std::error_condition condition = errorCode.default_error_condition();  //category().default_error_condition(value()).
		const std::string message = errorCode.message(); //category().message(value())
		return fmt::format( "({}){} - {})", value, category.name(), message );
	}
	string CodeException::ToString( const std::error_category& errorCategory )noexcept
	{
		return errorCategory.name();
	}
	std::string CodeException::ToString( const std::error_condition& errorCondition )noexcept
	{
		const int value = errorCondition.value();
		const std::error_category& category = errorCondition.category();
		const std::string message = errorCondition.message();
		return fmt::format( "({}){} - {})", value, category.name(), message );
	}

	RuntimeException::RuntimeException( const std::runtime_error& inner ):
		Exception( inner )
	{}

/*	ostream& operator<<(ostream& os, const Exception& dt)
	{
		os << _what.c_str();
		return os;
	}
	*/
}