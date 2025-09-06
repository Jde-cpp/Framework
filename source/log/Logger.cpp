#include <jde/framework/log/Logger.h>

namespace Jde{
	α Logging::CanBreak()ι->bool{ return Process::IsDebuggerPresent(); }

	α Logging::Log( const Entry& entry )ι->void{
		if( Process::Finalizing() || !ShouldLog(entry.Level, entry.Tags) )
			return;
		for( auto& logger : Logging::Loggers() ){
			try{
				if( logger->ShouldLog(entry.Level, entry.Tags) )
					logger->Write( entry );
			}
			catch( const fmt::format_error& e ){
				Jde::Critical{ ELogTags::App, "could not log entry '{}' error: '{}'", entry.Text, string{e.what()} };
			}
		}
	}
}