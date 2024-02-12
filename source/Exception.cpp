#include <jde/Exception.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <boost/system/error_code.hpp>

#include <jde/Log.h>
#include "io/ServerSink.h"
#include <jde/TypeDefs.h>
#define var const auto

namespace Jde
{
	IException::IException( string value, ELogLevel level, uint code, SL sl )ι:
		IException{ value, level, code, nullptr, sl }
	{}

	IException::IException( string value, ELogLevel level, uint code, sp<LogTag>&& tag, SL sl )ι:
		_stack{ sl },
		_what{ move(value) },
		Code( code ? code : Calc32RunTime(value) ),
		_pTag{ move(tag) },
		_level{ level }
	{
		BreakLog();
	}

	IException::IException( vector<string>&& args, string&& format, SL sl, uint c, ELogLevel level )ι:
		_stack{ sl },
		_format{ move(format) },
		_args{ move(args) },
		Code( c ? c : Calc32RunTime(format) ),
		_level{ level }
	{
		BreakLog();
	}

	IException::IException( IException&& from )ι:
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

	α IException::BreakLog()Ι->void
	{
#ifndef NDEBUG
		if( /*Level()!=ELogLevel::None &&*/ Level()>Logging::BreakLevel() )
		{
			Log();
			SetLevel( ELogLevel::NoLog );
			BREAK;
		}
#endif
	}
	α IException::Log()Ι->void
	{
		if( Level()==ELogLevel::NoLog || _pTag && Level()<_pTag->Level )
			return;
		var& sl = _stack.front();
		const string fileName{ strlen(sl.file_name()) ? FileName(sl.file_name()) : "{Unknown}\0"sv };
		var functionName = strlen(sl.file_name()) ? sl.function_name() : "{Unknown}\0";
		if( auto p = Logging::Default(); p )
			p->log( spdlog::source_loc{fileName.c_str(), (int)sl.line(), functionName}, (spdlog::level::level_enum)Level(), what() );
		if( Logging::Server::Enabled() )
			Logging::LogServer( Logging::Messages::ServerMessage{Logging::Message{Level(), what(), sl}, vector<string>{_args}} );
	}

	α IException::what()Ι->const char*
	{
		if( _what.empty() )
		{
			using ctx = fmt::format_context;
			vector<fmt::basic_format_arg<ctx>> args2;
			for( var& a : _args )
				args2.push_back( fmt::detail::make_arg<ctx>(a) );
			try
			{
				_what = fmt::vformat( _format, fmt::basic_format_args<ctx>(args2.data(), (int)args2.size()) );
			}
			catch( const fmt::format_error& e )
			{
				_what = format( "format error _format='{}' - {}", _format, e.what() );
			}
		}
		return _what.c_str();
	}

	CodeException::CodeException( string value, std::error_code&& code, ELogLevel level, SL sl )ι:
		IException{ move(value), level, (uint)code.value(), sl },
		_errorCode{ move(code) }
	{}

	CodeException::CodeException( std::error_code&& code, ELogLevel level, SL sl )ι:
		CodeException( format("{}-{}", code.value(), code.message()), move(code), level, sl )
	{}

	CodeException::CodeException( std::error_code&& code, sp<LogTag> tag, ELogLevel level, SL sl )ι:
		IException{ format("{}-{}", code.value(), code.message()), level, (uint)code.value(), move(tag), sl },
		_errorCode{ move(code) }
	{}

	BoostCodeException::BoostCodeException( const boost::system::error_code& c, sv msg, SL sl )ι:
		IException{ string{msg}, ELogLevel::Debug, (uint)c.value(), sl },
		_errorCode{ mu<boost::system::error_code>(c) }
	{}
	BoostCodeException::BoostCodeException( BoostCodeException&& e )ι:
		IException{ move(e) },
		_errorCode{ mu<boost::system::error_code>(*e._errorCode) }
	{}
	BoostCodeException::~BoostCodeException()
	{}

	α CodeException::ToString( const std::error_code& errorCode )ι->string
	{
		var value = errorCode.value();
		var& category = errorCode.category();
		var message = errorCode.message();
		return format( "({}){} - {})", value, category.name(), message );
	}

	α CodeException::ToString( const std::error_category& errorCategory )ι->string{	return errorCategory.name(); }

	α CodeException::ToString( const std::error_condition& errorCondition )ι->string
	{
		const int value = errorCondition.value();
		const std::error_category& category = errorCondition.category();
		const string message = errorCondition.message();
		return format( "({}){} - {})", value, category.name(), message );
	}

	OSException::OSException( TErrorCode result, string&& msg, SL sl )ι:
#ifdef _MSC_VER
		IException{ {std::to_string(result), std::to_string(GetLastError()), move(msg)}, "result={}/error={} - {}", sl, GetLastError() }
#else
		IException{ {std::to_string(result), std::to_string(errno), move(msg)}, "result={}/error={} - {}", sl, (uint)errno }
#endif
	{}

	Exception::Exception( string what, ELogLevel l, SL sl )ι:
		IException{ move(what), l, 0, sl }
	{}

	α IOException::Path()Ι->path
	{
		return  _pUnderLying? _pUnderLying->path1() : _path;
	}
	α IOException::SetWhat()Ι->void
	{
		_what = _pUnderLying ? _pUnderLying->what() : Code
			? format( "({}) {} - {} path='{}'", Code, std::strerror(errno), IException::what(), Path().string() )
			: format( "({}){}", Path().string(), IException::what() );
	}
	α IOException::what()Ι->const char*
	{
		return _what.c_str();
	}

	NetException::NetException( sv host, sv target, uint code, string result, ELogLevel level, SL sl )ι:
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

	α NetException::Log( string extra )Ι->void
	{
		if( Level()==ELogLevel::NoLog )
			return;
		var sl = _stack.front();
#ifndef NDEBUG
		if( string{sl.file_name()}.ends_with("construct_at") || Level()>=Logging::BreakLevel() )
			BREAK;
#endif
		if( auto p = Logging::Default(); p )
			p->log( spdlog::source_loc{FileName(sl.file_name()).c_str(), (int)sl.line(), sl.function_name()}, (spdlog::level::level_enum)Level(), extra.size() ? format("{}\n{}", extra, _what) : _what );
		SetLevel( ELogLevel::NoLog );
		//if( Logging::Server() )
		//	Logging::LogServer( Logging::Messages::Message{Logging::Message2{_level, _what, _sl.file_name(), _sl.function_name(), _sl.line()}, vector<string>{_args}} );

		try{
			var path = fs::temp_directory_path()/"ssl_error_response.json";
			auto l{ Threading::UniqueLock(path.string()) };
			std::ofstream os{ path };
			os << Result;
		}
		catch( std::exception& e ){
			ERRT( IO::Sockets::LogTag(), "could not save error result:  {}", e.what() );
		}
	}
}