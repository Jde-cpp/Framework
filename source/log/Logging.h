#pragma once
#include <array>
#include <memory>
#include <string_view>
#include <shared_mutex>
#include <sstream>
#include "../Exports.h"
#include "../io/Crc.h"
#include "../TypeDefs.h"
//#ifdef _MSC_VER
//	#include "server/EtwSink.h"
//#endif
namespace Jde
{
	using namespace std::literals::string_view_literals;
#pragma region ELogLevel
	enum class ELogLevel
	{
		Trace = 0,
		Debug = 1,
		Information = 2,
		Warning = 3,
		Error = 4,
		Critical = 5,
		None = 6
	};
	constexpr std::array<string_view,7> ELogLevelStrings = { "Trace"sv, "Debug"sv, "Information"sv, "Warning"sv, "Error"sv, "Critical"sv, "None"sv };
#pragma endregion
#pragma region MessageBase
	namespace IO{ class IncomingMessage; }
	namespace Logging
	{
		namespace Messages{ struct Message; }
		extern map<uint,set<uint>> OnceMessages; extern std::shared_mutex OnceMessageMutex;
		struct IServerSink;
#pragma region EFields
		enum class EFields : uint16
		{
			None=0,
			Timestamp=0x1,
			MessageId=0x2,
			Message=0x4,
			Level=0x8,
			FileId=0x10,
			File=0x20,
			FunctionId=0x40,
			Function=0x80,
			LineNumber=0x100,
			UserId=0x200,
			User=0x400,
			ThreadId=0x800,
			Thread=0x1000,
			VariableCount=0x2000,
			SessionId=0x4000
		};
		constexpr inline EFields operator|(EFields a, EFields b){ return (EFields)( (uint16)a | (uint16)b ); }
		constexpr inline EFields operator&(EFields a, EFields b){ return (EFields)( (uint16)a & (uint16)b ); }
		constexpr inline EFields operator~(EFields a){ return (EFields)( ~(uint16)a ); }
		constexpr inline EFields& operator|=(EFields& a, EFields b){ return a = a | b; }
		inline std::ostream& operator<<( std::ostream& os, const EFields& value ){ os << (uint)value; return os; }
#pragma endregion

		struct MessageBase
		{
			constexpr MessageBase( ELogLevel level, std::string_view message, std::string_view file, std::string_view function, uint line )noexcept;
			constexpr MessageBase( ELogLevel level, std::string_view message, std::string_view file, std::string_view function, uint line, uint messageId, uint fileId, uint functionId )noexcept;
			MessageBase( IO::IncomingMessage& message, EFields fields )noexcept(false);
			EFields Fields{EFields::None};
			ELogLevel Level;
			uint MessageId{0};
			string_view MessageView;
			uint FileId{0};
			string_view File;
			uint FunctionId{0};
			string_view Function;
			uint LineNumber;
			uint UserId{0};
			uint ThreadId{0};
		};
#pragma endregion
		void Log( const Logging::MessageBase& messageBase );
		JDE_NATIVE_VISIBILITY void LogCritical( const Logging::MessageBase& messageBase );
		template<class... Args >
		void Log( const Logging::MessageBase& messageBase, Args&&... args );

		JDE_NATIVE_VISIBILITY bool ShouldLogOnce( const Logging::MessageBase& messageBase );
		JDE_NATIVE_VISIBILITY void LogOnce( const Logging::MessageBase& messageBase );
		template<class... Args >
		void LogOnce( const Logging::MessageBase& messageBase, Args&&... args );
		//void LogOnceVec( const Logging::MessageBase& messageBase, const vector<string>& values );
		template<class... Args >
		void LogCritical( const Logging::MessageBase& messageBase, Args&&... args );
		template<class... Args >
		void LogNoServer( const Logging::MessageBase& messageBase );
		JDE_NATIVE_VISIBILITY void LogServer( const Logging::MessageBase& messageBase );
		JDE_NATIVE_VISIBILITY void LogServer( const Logging::MessageBase& messageBase, const vector<string>& values );
		JDE_NATIVE_VISIBILITY void LogServer( const Logging::Messages::Message& message );

		//JDE_NATIVE_VISIBILITY void LogEtw( const Logging::MessageBase& messageBase );
		//JDE_NATIVE_VISIBILITY void LogEtw( const Logging::MessageBase& messageBase, const vector<string>& values );
	}
}
// constexpr auto* GetFileName( const char* const path )
// {
// 	const auto* startPosition = path;
// 	for( const auto* currentCharacter = path;*currentCharacter != '\0'; ++currentCharacter )
// 	{
// 		if( *currentCharacter == '\\' || *currentCharacter == '/' )
// 			startPosition = currentCharacter;
// 	}
//     if( startPosition != path )
// 			++startPosition;
//     return startPosition;
// }
#define MY_FILE __FILE__

#define CRITICAL(message,...) Jde::Logging::LogCritical( Jde::Logging::MessageBase(Jde:: ELogLevel::Critical, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define CRITICAL0( message ) Jde::Logging::LogCritical( Jde::Logging::MessageBase(Jde::ELogLevel::Critical, message, MY_FILE, __func__, __LINE__) )
#define ERR0(message) Logging::Log( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__) )
#define ERR0_ONCE(message) Logging::LogOnce( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__) )
#define ERR(message,...) Logging::Log( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define ERRX(message,...) Logging::LogNoServer( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define ERRN( message, ... ) Logging::Log( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__, IO::Crc::Calc32RunTime(message), IO::Crc::Calc32RunTime(MY_FILE), IO::Crc::Calc32RunTime(__func__)), __VA_ARGS__ )
#define ERR_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define WARN0(message) Logging::Log( Logging::MessageBase(ELogLevel::Warning, message, MY_FILE, __func__, __LINE__) )
#define WARN0N( message ) Logging::Log( Logging::MessageBase(ELogLevel::Warning, message, MY_FILE, __func__, __LINE__, IO::Crc::Calc32RunTime(message), IO::Crc::Calc32RunTime(MY_FILE), IO::Crc::Calc32RunTime(__func__)) )
#define WARN(message,...) Logging::Log( Logging::MessageBase(ELogLevel::Warning, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define WARNN( message, ... ) Logging::Log( Logging::MessageBase(ELogLevel::Warning, message, MY_FILE, __func__, __LINE__, IO::Crc::Calc32RunTime(message), IO::Crc::Calc32RunTime(MY_FILE), IO::Crc::Calc32RunTime(__func__)), __VA_ARGS__ )
#define INFO0(message) Logging::Log( Logging::MessageBase(ELogLevel::Information, message, MY_FILE, __func__, __LINE__) )
#define INFO(message,...) Jde::Logging::Log( Jde::Logging::MessageBase(Jde::ELogLevel::Information, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define INFON( message, ... ) Logging::Log( Logging::MessageBase(ELogLevel::Information, message, MY_FILE, __func__, __LINE__, IO::Crc::Calc32RunTime(message), IO::Crc::Calc32RunTime(MY_FILE), IO::Crc::Calc32RunTime(__func__)), __VA_ARGS__ )
#define INFO0_ONCE(message) Logging::LogOnce( Logging::MessageBase(ELogLevel::Information, message, MY_FILE, __func__, __LINE__) )
#define DBG(message,...) Jde::Logging::Log( Jde::Logging::MessageBase(Jde::ELogLevel::Debug, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define DBG0( message ) Logging::Log( Logging::MessageBase(ELogLevel::Debug, message, MY_FILE, __func__, __LINE__) )
#define DBGN( message, ... ) Logging::Log( Logging::MessageBase(ELogLevel::Debug, message, MY_FILE, __func__, __LINE__, IO::Crc::Calc32RunTime(message), IO::Crc::Calc32RunTime(MY_FILE), IO::Crc::Calc32RunTime(__func__)), __VA_ARGS__ )
#define DBGX(message,...) Logging::LogNoServer( Logging::MessageBase(ELogLevel::Debug, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define TRACE(message,...) Logging::Log( Logging::MessageBase(ELogLevel::Trace, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define TRACE0(message) Logging::Log( Logging::MessageBase(ELogLevel::Trace, message, MY_FILE, __func__, __LINE__) )
#define TRACEX(message,...) Logging::LogNoServer( Logging::MessageBase(ELogLevel::Trace, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define TRACE0X(message) Logging::LogNoServer( Logging::MessageBase(ELogLevel::Trace, message, MY_FILE, __func__, __LINE__) )
#define LOG(severity,message,...) Logging::Log( Logging::MessageBase(severity, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define LOG0(severity,message) Logging::Log( Logging::MessageBase(severity, message, MY_FILE, __func__, __LINE__) )
//#define LOG_SQL(sql,pParams) 

namespace spdlog
{ 
#ifdef _MSC_VER
	namespace level{ enum level_enum : int;}
#endif
} 
namespace Jde
{
	extern std::shared_ptr<spdlog::logger> spLogger;
	JDE_NATIVE_VISIBILITY void DestroyLogger();

   using namespace std::literals;
	JDE_NATIVE_VISIBILITY void InitializeLogger( string_view fileName )noexcept;
	JDE_NATIVE_VISIBILITY void InitializeLogger( ELogLevel level2=ELogLevel::Debug, const fs::path& path=fs::path{}, bool sendToServer=false )noexcept;
	JDE_NATIVE_VISIBILITY bool HaveLogger()noexcept;
#if _MSC_VER
	JDE_NATIVE_VISIBILITY spdlog::logger* GetDefaultLogger()noexcept;
	JDE_NATIVE_VISIBILITY Logging::IServerSink* GetServerSink()noexcept;
	JDE_NATIVE_VISIBILITY void SetServerSink( Logging::IServerSink* p )noexcept;
	namespace Logging{ struct EtwSink; }
	extern std::shared_ptr<Logging::EtwSink> _spEtwSink;
	extern Logging::EtwSink* _pEtwSink;
//	JDE_NATIVE_VISIBILITY Logging::EtwSink* GetEtwSink();
#else
	extern spdlog::logger* pLogger;
	extern Logging::IServerSink* _pServerSink;
	inline spdlog::logger* GetDefaultLogger()noexcept{ /*assert(pLogger);*/ return pLogger; }
	inline Logging::IServerSink* GetServerSink()noexcept{ return _pServerSink; }
	inline void SetServerSink( Logging::IServerSink* p )noexcept{_pServerSink=p;}

	namespace Logging{ struct Lttng; }
	extern Logging::Lttng* _pLttng;
	extern sp<Logging::Lttng> _spLttng;
//	JDE_NATIVE_VISIBILITY Logging::Lttng* GetEtwSink();
#endif	 
	//spd::stdout_color_mt
//	using std::string;
	JDE_NATIVE_VISIBILITY std::ostream& operator<<( std::ostream& os, const ELogLevel& value );
//https://stackoverflow.com/questions/21806561/concatenating-strings-and-numbers-in-variadic-template-function
	namespace ToVec
	{
		inline void Append( vector<string>& /*values*/ ){}

    	template<typename Head, typename... Tail>
    	void Append( vector<string>& values, Head&& h, Tail&&... t );

		template<typename... Tail>
		void Append( vector<string>& values, std::string&& h, Tail&&... t )
		{
			values.push_back( h );
			return Apend( values, std::forward<Tail>(t)... );
		}

    	template<typename T>
    	std::string to_string( const T& x )
    	{
			std::ostringstream os;
			os << x;
      	return os.str();
    	}

    	template<typename Head, typename... Tail>
    	void Append( vector<string>& values, Head&& h, Tail&&... t )
    	{
			values.push_back( to_string(std::forward<Head>(h)) );
      	return Append( values, std::forward<Tail>(t)... );
    	}
	}
	namespace Logging
	{
		namespace Proto{class Status;}
		TimePoint StartTime()noexcept;
		JDE_NATIVE_VISIBILITY void SetStatus( const vector<string>& values )noexcept;
		void SetLogLevel( ELogLevel client, ELogLevel server )noexcept;
		Proto::Status* GetAllocatedStatus()noexcept;
		inline void Log( const Logging::MessageBase& messageBase )
		{
			GetDefaultLogger()->log( ( spdlog::level::level_enum)messageBase.Level, messageBase.MessageView );
			if( GetServerSink() )
				LogServer( messageBase );
			// if( GetEtwSink() )
			// 	LogEtw( messageBase );
		}

		template<class... Args >
		inline void LogCritical( const Logging::MessageBase& messageBase, Args&&... args )
		{
			GetDefaultLogger()->log( spdlog::level::level_enum::critical, messageBase.MessageView.data(), args... );
#ifdef NDEBUG
 			LogCritical( Jde::Logging::MessageBase(Jde::ELogLevel::Critical, messageBase.MessageView, "Logging.h", "LogCritical", 218) );
#endif
			if( GetServerSink() )
			{
				vector<string> values; values.reserve( sizeof...(args) );
				ToVec::Append( values, args... );
				if( GetServerSink() )
					LogServer( messageBase, values );
			}
			// if( GetEtwSink() )
			// {
			// 	vector<string> values; values.reserve( sizeof...(args) );
			// 	ToVec::Append( values, args... );
			// 	LogEtw( messageBase, values );
			// }
		}
		
		template<class... Args >
		inline void Log( const Logging::MessageBase& messageBase, Args&&... args )
		{
			if( pLogger )
				GetDefaultLogger()->log( (spdlog::level::level_enum)messageBase.Level, messageBase.MessageView.data(), args... );
			if( GetServerSink() )
			{
				vector<string> values; values.reserve( sizeof...(args) );
				ToVec::Append( values, args... );
				LogServer( messageBase, values );
			}
/*			if( GetEtwSink() )
			{
				vector<string> values; values.reserve( sizeof...(args) );
				ToVec::Append( values, args... );
				LogEtw( messageBase, values );
			}*/
		}
		template<class... Args >
		void LogOnce( const Logging::MessageBase& messageBase, Args&&... args )
		{
			if( ShouldLogOnce(messageBase) )
				Log( messageBase, args... );
		}

		inline void LogNoServer( const Logging::MessageBase& messageBase )
		{
			GetDefaultLogger()->log( (spdlog::level::level_enum)messageBase.Level, messageBase.MessageView );
		}

		template<class... Args >
		inline void LogNoServer( const Logging::MessageBase& messageBase, Args&&... args )
		{
			GetDefaultLogger()->log( (spdlog::level::level_enum)messageBase.Level, messageBase.MessageView.data(), args... );
		}
	}
}
JDE_NATIVE_VISIBILITY std::ostream& operator<<( std::ostream& os, const std::optional<double>& value );

#pragma region MessageBase
namespace Jde::Logging
{
	constexpr MessageBase::MessageBase( ELogLevel level, std::string_view message, std::string_view file, std::string_view function, uint line, uint messageId, uint fileId, uint functionId )noexcept:
		Level{level},
		MessageId{messageId},//{IO::Crc::Calc32(message)},
		MessageView{message},
		FileId{ fileId },
		File{file},
		FunctionId{ functionId },
		Function{function},
		LineNumber{line}
	{
		if( level!=ELogLevel::Trace )
			Fields |= EFields::Level;
		if( message.size() )
			Fields |= EFields::Message | EFields::MessageId;
		if( File.size() )
			Fields |= EFields::File | EFields::FileId;
		if( Function.size() )
			Fields |= EFields::Function | EFields::FunctionId;
		if( LineNumber )
			Fields |= EFields::LineNumber;
		//if( message.size()==0 )
			//static_assert(IO::Crc::Calc32(message)==0, "Test constexpr");
	}
	constexpr MessageBase::MessageBase( ELogLevel level, std::string_view message, std::string_view file, std::string_view function, uint line )noexcept:
		MessageBase( level, message, file, function, line, IO::Crc::Calc32(message), IO::Crc::Calc32(file), IO::Crc::Calc32(function) )
	{}

}
#pragma endregion
