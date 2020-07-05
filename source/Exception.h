#pragma once
//#include <boost/system/error_code.hpp>

//#include <string>
#include "log/Logging.h"
#ifndef  THROW
# define THROW(x) Jde::throw_exception(x, __func__,__FILE__,__LINE__)
#endif
#ifndef  THROW_IF
# define THROW_IF(condition,x) if(condition) Jde::throw_exception(x, __func__,__FILE__,__LINE__)
#endif

//mysql undefs THROW :(
#ifndef  THROW2
# define THROW2(x) Jde::throw_exception(x, __func__,__FILE__,__LINE__)
#endif

#ifndef CATCH_STD
# define CATCH_STD(pszAdditional) catch (const std::exception& e){Jde::catch_exception(__func__,__FILE__,__LINE__,pszAdditional, &e); }
#endif

#ifndef CATCH_ALL
# define CATCH_ALL(pszAdditional) catch (...){Jde::catch_exception(__func__,__FILE__,__LINE__,pszAdditional); }
#endif

#ifndef CATCH_DEFAULT
# define CATCH_DEFAULT(pszAdditional) catch (const Exception&){} CATCH_STD(pszAdditional) CATCH_ALL(pszAdditional)
#endif

namespace Jde
{
	struct JDE_NATIVE_VISIBILITY Exception : public std::exception
	{
		Exception()=default;
		Exception( const Exception& )=default;
		Exception( Exception&& )=default;
		Exception( ELogLevel level, string_view value );
		Exception( ELogLevel level, string_view value, string_view function, string_view file, long line );
		Exception( std::string_view value );
		template<class... Args>
		Exception( std::string_view value, Args&&... args );

		Exception( const std::exception& exp );

		virtual ~Exception();

		//Exception& operator=( const Exception& copyFrom );
		virtual void Log( std::string_view pszAdditionalInformation="", ELogLevel level=ELogLevel::Debug )const;
		const char* what() const noexcept override;
		ELogLevel GetLevel()const{return _level;}

		//shared_ptr<spdlog::logger> GetLogger()const{return _pLogger;}
		void SetFunction( const char* pszFunction ){ _functionName = pszFunction; }
		void SetFile( const char* pszFile ){ _fileName = pszFile; }
		void SetLine( long line ){ _line = line; }
		friend std::ostream& operator<<( std::ostream& os, const Exception& e );
	protected:
		std::string _functionName;
		std::string _fileName;
		long _line;

		ELogLevel _level{ELogLevel::Trace};
		mutable std::string _what;
	private:
		std::string _format;
		//uint32 _messageId;
		vector<string> _args;
	};


	template<class... Args>
	Exception::Exception( std::string_view value, Args&&... args ):
		_format{ value },
		_what{ fmt::format(value,args...) }
	{
		_args.reserve( sizeof...(args) );
		ToVec::Append( _args, args... );
	}

	//Before program runs
	struct JDE_NATIVE_VISIBILITY LogicException : public Exception
	{
		template<class... Args>
		LogicException( std::string_view value, Args&&... args ):
			Exception( value, args... )
		{
			_level=ELogLevel::Critical;
		}
	};
	//environment variables
	struct JDE_NATIVE_VISIBILITY EnvironmentException : public Exception
	{
		template<class... Args>
		EnvironmentException( std::string_view value, Args&&... args ):
			Exception( value, args... )
		{
			_level = ELogLevel::Error;
		}

	};


	//errors detectable when the program executes
	struct JDE_NATIVE_VISIBILITY RuntimeException : public Exception
	{
		RuntimeException()=default;
		RuntimeException( const std::runtime_error& inner );
		template<class... Args>
		RuntimeException( std::string_view value, Args&&... args ):
			Exception( value, args... )
		{}
	};

	struct JDE_NATIVE_VISIBILITY CodeException : public RuntimeException
	{
		CodeException( std::string_view value, const std::error_code& code, ELogLevel level=ELogLevel::Error );

		static std::string ToString( const std::error_code& pErrorCode )noexcept;
		static std::string ToString( const std::error_category& errorCategory )noexcept;
		static std::string ToString( const std::error_condition& errorCondition )noexcept;
	private:
		std::shared_ptr<std::error_code> _pErrorCode;
	};

	//https://stackoverflow.com/questions/10176471/is-it-possible-to-convert-a-boostsystemerror-code-to-a-stderror-code

	struct JDE_NATIVE_VISIBILITY BoostCodeException : public RuntimeException
	{
		BoostCodeException( const boost::system::error_code& ec );
	private:
		boost::system::error_code _errorCode;
	};


	struct JDE_NATIVE_VISIBILITY ArgumentException : public RuntimeException
	{
		template<class... Args>
		ArgumentException( std::string_view value, Args&&... args ):
			RuntimeException( value, args... )
		{}
	};

	struct JDE_NATIVE_VISIBILITY IOException : public RuntimeException
	{
		template<class... Args>
		IOException( std::string_view value, Args&&... args ):
			RuntimeException( value, args... )
		{}

		template<class... Args>
		IOException( uint errorCode, std::string_view value, Args&&... args ):
			RuntimeException( value, args... ),
			ErrorCode{errorCode}
		{}
		const char* what() const noexcept override;
		const uint ErrorCode{0};
		fs::path Path;
	};

	template<class TException>
	[[noreturn]] void throw_exception( TException exp, const char* pszFunction, const char* pszFile, long line )
	{
		exp.SetFunction( pszFunction );
		exp.SetFile( pszFile );
		exp.SetLine( line );
		exp.Log();
		throw exp;
	}

	JDE_NATIVE_VISIBILITY void catch_exception( std::string_view pszFunction, std::string_view pszFile, long line, std::string_view pszAdditional, const std::exception* pException=nullptr );
	//https://stackoverflow.com/questions/35941045/can-i-obtain-c-type-names-in-a-constexpr-way/35943472#35943472
	template<class T>
	constexpr std::basic_string_view<char> GetTypeName()
	{
#ifdef _WINDOWS
		char const* p = __FUNCSIG__;
#else
		char const* p = __PRETTY_FUNCTION__;
#endif
		while (*p++ != '=');
		for( ; *p == ' '; ++p );
		char const* p2 = p;
		int count = 1;
		for (;;++p2)
		{
			switch (*p2)
			{
			case '[':
				++count;
				break;
			case ']':
				--count;
				if (!count)
					return {p, std::size_t(p2 - p)};
			}
		}
//		return "";
	}

	inline bool Try( std::function<void()> func )
	{
		bool result = false;
		try
		{
			func();
			result = true;
		}
		catch( const Exception&)
		{}
		return result;
	}
	template<typename T>
	bool Try( std::function<T()> func )
	{
		bool result = false;
		try
		{
			func();
			result = true;
		}
		catch( const Exception&)
		{}
		return result;
	}


}