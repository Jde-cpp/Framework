#include <jde/framework/log/MemoryLog.h>

#define let const auto
namespace Jde{
	Ω memoryLog()ε->Logging::MemoryLog&{
		for( let& logger : Logging::Loggers() ){
			if( let pMemoryLog = dynamic_cast<Logging::MemoryLog*>(logger.get()); pMemoryLog )
				return *pMemoryLog;
		}
		throw Exception( "No MemoryLog logger registered." );
	}
	α Logging::ClearMemory()ι->void{ memoryLog().Clear(); }
	α Logging::Find( StringMd5 entryId )ι->vector<Logging::Entry>{ return memoryLog().Find( entryId ); }
	α Logging::Find( function<bool(const Logging::Entry&)> f )ι->vector<Logging::Entry>{ return memoryLog().Find( f ); }
}
namespace Jde::Logging{
	α MemoryLog::Write( const Entry& m )ι->void{
		_entries.emplace_back( move(m) );
	}
	α MemoryLog::Write( ILogger& logger )ι->void{
		_entries.visit( [&](const auto& entry){ logger.Write( Entry{entry} ); } );
	}
	α MemoryLog::Find( StringMd5 id )ι->vector<Logging::Entry>{
		vector<Logging::Entry> result;
		_entries.visit( [&](const auto& entry){
			if( entry.Id() == id )
				result.push_back(entry);
		});
		return result;
	}
	α MemoryLog::Find( function<bool(const Logging::Entry&)> f )ι->vector<Logging::Entry>{
		vector<Logging::Entry> result;
		_entries.visit( [&](const auto& entry){
			if( f(entry) )
				result.push_back(entry);
		});
		return result;
	}
}