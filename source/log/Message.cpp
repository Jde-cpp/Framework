#include <jde/framework/log/Message.h>
#include <jde/framework/settings.h>

namespace Jde{
	optional<ELogLevel> _breakLevel;
	α Logging::BreakLevel()ι->ELogLevel{
		if( !_breakLevel )
			_breakLevel = Settings::FindEnum<ELogLevel>( "/logging/breakLevel", ToLogLevel ).value_or( ELogLevel::Warning );
		return *_breakLevel;
	}
}

α Jde::CanBreak()ι->bool{ return Process::IsDebuggerPresent(); }
namespace Jde::Logging{
	MessageBase::MessageBase( ELogLevel level, SL sl )ι:
		Fields{ EFields::File | EFields::FileId | EFields::Function | EFields::FunctionId | EFields::LineNumber },
		Level{ level },
		MessageId{ 0 },//{},
		FileId{ Calc32RunTime(FileName(sl.file_name())) },
		File{ sl.file_name() },
		FunctionId{ Calc32RunTime(sl.function_name()) },
		Function{ sl.function_name() },
		LineNumber{ sl.line() }{
		if( level!=ELogLevel::Trace )
			Fields |= EFields::Level;
	}
	MessageBase::MessageBase( ELogLevel level, sv message, const char* file, const char* function, uint_least32_t line )ι:
		Fields{ EFields::Message | EFields::File | EFields::FileId | EFields::Function | EFields::FunctionId | EFields::LineNumber },
		Level{ level },
		MessageId{ Calc32RunTime(message) },
		MessageView{ message },
		FileId{ Calc32RunTime(FileName(file)) },
		File{ file },
		FunctionId{ Calc32RunTime(function) },
		Function{ function },
		LineNumber{ line }
	{}
	MessageBase::MessageBase( ELogLevel level, ID messageId, ID fileId, ID functionId, uint_least32_t line, Jde::UserPK userPK, ThreadID threadId )ι:
		Fields{ EFields::MessageId | EFields::FileId | EFields::FunctionId | EFields::LineNumber | EFields::UserPK | EFields::ThreadId },
		Level{ level },
		MessageId{ messageId },
		FileId{ fileId },
		FunctionId{ functionId },
		LineNumber{ line },
		UserPK{ userPK },
		ThreadId{ threadId }
	{}


	Message::Message( const MessageBase& b )ι:
		MessageBase{ b },
		_fileName{ FileName(b.File) }
	{
		File = _fileName.c_str();
	}

	Message::Message( ELogLevel level, string message, SL sl )ι:
		MessageBase( level, sl ),
		_pMessage{ mu<string>(move(message)) },
		_fileName{ FileName(sl.file_name()) }{
		File = _fileName.c_str();
		MessageView = *_pMessage;
		MessageId = Calc32RunTime( MessageView );
	}

	Message::Message( sv tag, ELogLevel level, string message, SL sl )ι:
		MessageBase( level, sl ),
		Tag{ tag },
		_pMessage{ mu<string>(move(message)) },
		_fileName{ FileName(sl.file_name()) }{
		File = _fileName.c_str();
		MessageView = *_pMessage;
		MessageId = Calc32RunTime( MessageView );
	}

	Message::Message( sv tag, ELogLevel level, string message, char const* file, char const * function, boost::uint_least32_t line )ι:
		MessageBase{ level, message, file, function, line },
		Tag{ tag },
		_pMessage{ mu<string>(move(message)) },
		_fileName{ FileName(file) }{
		File = _fileName.c_str();
		MessageView = *_pMessage;
		MessageId = Calc32RunTime( MessageView );
		ASSERT( level>=ELogLevel::NoLog && level<=ELogLevel::Critical );
	}

	Message::Message( const Message& x )ι:
		MessageBase{ x },
		Tag{ x.Tag },
		_pMessage{ x._pMessage ? mu<string>(*x._pMessage) : nullptr },
		_fileName{ x._fileName }{
		File = _fileName.c_str();
		if( _pMessage ){
			MessageView = *_pMessage;
			MessageId = Calc32RunTime( MessageView );
		}
	}
}
