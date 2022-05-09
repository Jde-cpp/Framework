﻿#include <jde/Exception.h>
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
	IException::IException( string value, ELogLevel level, uint code, SL sl )noexcept:
		_stack{ sl },
		_what{ move(value) },
		Code( code ? code : Calc32RunTime(value) ),
		_level{ level }
	{
		BreakLog();
	}

	IException::IException( vector<string>&& args, string&& format, SL sl, uint c, ELogLevel level )noexcept:
		_stack{ sl },
		_format{ move(format) },
		_args{ move(args) },
		Code( c ? c : Calc32RunTime(format) ),
		_level{ level }
	{
		BreakLog();
	}

	IException::IException( IException&& from )noexcept:
		_stack{ move(from._stack) },
		_what{ move(from._what) },
		_pInner{ move(from._pInner) },
		_format{ move(from._format) },
		_args{ move(from._args) },
		Code{ from.Code },
		_level{ from.Level() }
	{
		BreakLog();
		ASSERT( _stack.stack.size() );
		from.SetLevel( ELogLevel::NoLog );
	}
	IException::~IException()
	{
		Log();
	}

	α IException::BreakLog()const noexcept->void
	{
//#ifndef NDEBUG
		if( Level()!=ELogLevel::None && Level()>Logging::BreakLevel() )
		{
			Log();
			BREAK;
		}
//#endif
	}
	α IException::Log()const noexcept->void
	{
		if( Level()==ELogLevel::NoLog )
			return;
		var& sl = _stack.front();
		const string fileName{ strlen(sl.file_name()) ? FileName(sl.file_name()) : "{Unknown}\0"sv };
		var functionName = strlen(sl.file_name()) ? sl.function_name() : "{Unknown}\0";
		Logging::Default().log( spdlog::source_loc{fileName.c_str(), (int)sl.line(), functionName}, (spdlog::level::level_enum)Level(), what() );
		if( Logging::Server() )
			Logging::LogServer( Logging::Messages::ServerMessage{Logging::Message{Level(), what(), sl}, vector<string>{_args}} );
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

	CodeException::CodeException( string value, std::error_code&& code, ELogLevel level, SL sl ):
		IException{ move(value), level, (uint)code.value(), sl },
		_errorCode{ move(code) }
	{
	}

	CodeException::CodeException( std::error_code&& code, ELogLevel level, SL sl ):
		CodeException( format("{}-{}", code.value(), code.message()), move(code), level, sl )
	{}

	BoostCodeException::BoostCodeException( const boost::system::error_code& c, sv msg, SL sl )noexcept:
		IException{ string{msg}, ELogLevel::Debug, (uint)c.value(), sl },
		_errorCode{ make_unique<boost::system::error_code>(c) }
	{}
	BoostCodeException::BoostCodeException( BoostCodeException&& e )noexcept:
		IException{ move(e) },
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

	OSException::OSException( TErrorCode result, string&& msg, SL sl )noexcept:
#ifdef _MSC_VER
		IException{ {std::to_string(result), std::to_string(GetLastError()), move(msg)}, "result={}/error={} - {}", sl, GetLastError() }
#else
		IException{ {std::to_string(result), std::to_string(errno), move(msg)}, "result={}/error={} - {}", sl, (uint)errno }
#endif
	{}

	Exception::Exception( string what, ELogLevel l, SL sl )noexcept:
		IException{ move(what), l, 0, sl }
	{}

	α IOException::Path()const noexcept->path
	{
		return  _pUnderLying? _pUnderLying->path1() : _path;
	}
	α IOException::SetWhat()const noexcept->void
	{
		_what = _pUnderLying ? _pUnderLying->what() : Code
			? format( "({}) {} - {} path='{}'", Code, std::strerror(errno), IException::what(), Path().string() )
			: format( "({}){}", Path(), IException::what() );
	}
	α IOException::what()const noexcept->const char*
	{
		return _what.c_str();
	}

	NetException::NetException( sv host, sv target, uint code, string result, ELogLevel level, SL sl )noexcept:
		IException{ {string{host}, string{target}, std::to_string(code), string{result.substr(0,100)}}, "{}{} ({}){}", sl, code }, //"
		Host{ host },
		Target{ target },
		//Code{ code },
		Result{ move(result) }
	{
		SetLevel( level );
		_what = format( "{}{} ({}){}", Host, Target, code, Result );
		if( var f = Settings::Get<fs::path>("net/errorFile"); f )
		{
			std::ofstream os{ *f };
			os << Result;
		}
		//Log();
	}

	α NetException::Log( string extra )const noexcept->void
	{
		if( Level()==ELogLevel::NoLog )
			return;
		var sl = _stack.front();
#ifndef NDEBUG
		if( string{sl.file_name()}.ends_with("construct_at") || Level()>=Logging::BreakLevel() )
			BREAK;
#endif
		Logging::Default().log( spdlog::source_loc{FileName(sl.file_name()).c_str(), (int)sl.line(), sl.function_name()}, (spdlog::level::level_enum)Level(), extra.size() ? format("{}\n{}", extra, _what) : _what );
		SetLevel( ELogLevel::NoLog );
		//if( Logging::Server() )
		//	Logging::LogServer( Logging::Messages::Message{Logging::Message2{_level, _what, _sl.file_name(), _sl.function_name(), _sl.line()}, vector<string>{_args}} );

		try
		{
			var path = fs::temp_directory_path()/"ssl_error_response.json";
			auto l{ Threading::UniqueLock(path.string()) };
			std::ofstream os{ path };
			os << Result;
		}
		catch( std::exception& e )
		{
			ERR( "could not save error result:  {}", e.what() );
		}
	}
}