#include <jde/Exception.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <boost/system/error_code.hpp>

#include <jde/log/Log.h>
//#include "io/ServerSink.h"
#include <jde/TypeDefs.h>
#define var const auto

namespace Jde{
	up<IException> _empty;
	α IException::EmptyPtr()ι->const up<IException>&{ return _empty; }

	IException::IException( string value, ELogLevel level, uint code, SL sl )ι:
		IException{ value, level, code, nullptr, sl }
	{}

	IException::IException( string value, ELogLevel level, uint code, sp<LogTag>&& tag, SL sl )ι:
		_stack{ sl },
		_what{ move(value) },
		_pTag{ move(tag) },
		Code( code ? code : Calc32RunTime(value) ),
		_level{ level }{
		BreakLog();
	}

	IException::IException( const IException& from )ι:
		_stack{ from._stack },
		_what{ from._what },
		_pInner{ from._pInner },
		_format{ from._format },
		_pTag{ from._pTag },
		_args{ from._args },
		Code{ from.Code },
		_level{ from.Level() }{
		BREAK;//should only be called by rethrow_exception
//		from._level = ELogLevel::NoLog;
	};
	IException::IException( IException&& from )ι:
		_stack{ move(from._stack) },
		_what{ move(from._what) },
		_pInner{ move(from._pInner) },
		_format{ move(from._format) },
		_pTag{ from._pTag },
		_args{ move(from._args) },
		Code{ from.Code },
		_level{ from.Level() }{
		BreakLog();
		ASSERT( _stack.stack.size() );
		from.SetLevel( ELogLevel::NoLog );
	}
	IException::~IException(){
		Log();
	}

	α IException::FromExceptionPtr( const std::exception_ptr& from, SL sl )ι->up<IException>{
		up<IException> y;
		try{
			std::rethrow_exception( from );
		}
		catch( IException& e ){
			y = e.Move();
		}
		catch( const nlohmann::json::exception& e ){
			y = mu<Exception>( Jde::format("nlohmann::json::exception - {}", e.what()), ELogLevel::Error, sl );
		}
		catch( const std::exception& e ){
			y = mu<Exception>( Jde::format("std::exception - {}", e.what()), ELogLevel::Critical, sl );
		}
		catch( ... ){
			y = mu<Exception>( "unknown exception", ELogLevel::Critical, sl );
		}
		return y;
	}

	α IException::BreakLog()Ι->void{
#ifndef NDEBUG
		if( /*Level()!=ELogLevel::None &&*/ Level()>=Logging::BreakLevel() ){
			Log();
			SetLevel( ELogLevel::NoLog );
		}
#endif
	}
	α IException::Log()Ι->void{
		if( Level()==ELogLevel::NoLog || (_pTag && Level()<MinLevel(ToLogTags(_pTag->Id))) )
			return;
		var& sl = _stack.front();
		const string fileName{ strlen(sl.file_name()) ? FileName(sl.file_name()) : "{Unknown}\0"sv };
		var functionName = strlen(sl.file_name()) ? sl.function_name() : "{Unknown}\0";
		if( auto p = Logging::Default(); p )
			p->log( spdlog::source_loc{fileName.c_str(), (int)sl.line(), functionName}, (spdlog::level::level_enum)Level(), what() );
		if( Logging::External::Size() )
			Logging::External::Log( Logging::ExternalMessage{Logging::Message{Level(), what(), sl}, vector<string>{_args}} );
		if( Logging::LogMemory() ){
			if( _format.size() )
				Logging::LogMemory( Logging::Message{Level(), string{_format}, sl}, vector<string>{_args} );
			else
				Logging::LogMemory( Logging::Message{Level(), _what, sl} );
		}
	}

	α IException::what()Ι->const char*{
		if( _what.empty() )
			_what = Str::ToString( _format, _args );
		return _what.c_str();
	}

	CodeException::CodeException( std::error_code code, sp<LogTag> tag, string value, ELogLevel level, SL sl )ι:
		IException{ Jde::format("({}){} - {}", code.value(), value, code.message()), level, (uint)code.value(), move(tag), sl },
		_errorCode{ move(code) }
	{}

	CodeException::CodeException( std::error_code code, sp<LogTag> tag, ELogLevel level, SL sl )ι:
		IException{ Jde::format("({}){}", code.value(), code.message()), level, (uint)code.value(), move(tag), sl },
		_errorCode{ move(code) }
	{}

	CodeException::CodeException( std::error_code code, ELogTags tags, ELogLevel level, SL sl )ι:
		IException{ tags, sl, level, "({}){}", code.value(), code.message() },
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

	α CodeException::ToString( const std::error_code& errorCode )ι->string{
		var value = errorCode.value();
		var& category = errorCode.category();
		var message = errorCode.message();
		return Jde::format( "({}){} - {})", value, category.name(), message );
	}

	α CodeException::ToString( const std::error_category& errorCategory )ι->string{	return errorCategory.name(); }

	α CodeException::ToString( const std::error_condition& errorCondition )ι->string{
		const int value = errorCondition.value();
		const std::error_category& category = errorCondition.category();
		const string message = errorCondition.message();
		return Jde::format( "({}){} - {})", value, category.name(), message );
	}

	OSException::OSException( TErrorCode result, string&& m, SL sl )ι:
#ifdef _MSC_VER
		IException{ {std::to_string(result), std::to_string(GetLastError()), move(msg)}, "result={}/error={} - {}", sl, GetLastError() }
#else
		IException{ sl, ELogLevel::Error, (uint)errno, "result={}/error={} - {}", result, errno, m }
#endif
	{}

	Exception::Exception( string what, ELogLevel l, SL sl )ι:
		IException{ move(what), l, 0, sl }
	{}

	α IOException::Path()Ι->const fs::path&{
		return  _pUnderLying? _pUnderLying->path1() : _path;
	}
	α IOException::SetWhat()Ι->void{
		_what = _pUnderLying ? _pUnderLying->what() : Code
			? Jde::format( "({}) {} - {} path='{}'", Code, std::strerror(errno), IException::what(), Path().string() )
			: Jde::format( "({}){}", Path().string(), IException::what() );
	}
	α IOException::what()Ι->const char*{
		return _what.c_str();
	}

// 	NetException::NetException( sv host, sv target, uint code, string result, ELogLevel level, SL sl )ι:
// //		IException{ {string{host}, string{target}, std::to_string(code), string{result.substr(0,100)}}, "{}{} ({}){}", sl, code }, //"
// 		IException{ sl, level, code, "{}{} ({}){}", host, target, code, result.substr(0,100) }, //"
// 		Host{ host },
// 		Target{ target },
// 		//Code{ code },
// 		Result{ move(result) }{
// 		SetLevel( level );
// 		_what = Jde::format( "{}{} ({}){}", Host, Target, code, Result );
// 		if( var f = Settings::Get<fs::path>("net/errorFile"); f ){
// 			std::ofstream os{ *f };
// 			os << Result;
// 		}
// 		//Log();
// 	}

// 	α NetException::Log( string extra )Ι->void{
// 		if( Level()==ELogLevel::NoLog )
// 			return;
// 		var sl = _stack.front();
// #ifndef NDEBUG
// 		if( string{sl.file_name()}.ends_with("construct_at") || Level()>=Logging::BreakLevel() )
// 			BREAK;
// #endif
// 		if( auto p = Logging::Default(); p )
// 			p->log( spdlog::source_loc{FileName(sl.file_name()).c_str(), (int)sl.line(), sl.function_name()}, (spdlog::level::level_enum)Level(), extra.size() ? Jde::format("{}\n{}", extra, _what) : _what );
// /*		SetLevel( ELogLevel::NoLog );
// 		//if( Logging::Server() )
// 		//	Logging::LogServer( Logging::Messages::Message{Logging::Message2{_level, _what, _sl.file_name(), _sl.function_name(), _sl.line()}, vector<string>{_args}} );

// 		try{
// 			var path = fs::temp_directory_path()/"ssl_error_response.json";
// 			auto l{ Threading::UniqueLock(path.string()) };
// 			std::ofstream os{ path };
// 			os << Result;
// 		}
// 		catch( std::exception& e ){
// 			ERRT( IO::Sockets::LogTag(), "could not save error result:  {}", e.what() );
// 		}*/
// 	}
}