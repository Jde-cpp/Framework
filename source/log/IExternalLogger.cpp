#include "jde/framework/log/IExternalLogger.h"

namespace Jde::Logging{
	constexpr ELogTags _tags{ ELogTags::ExternalLogger };
	vector<up<Logging::IExternalLogger>> _externalLoggers;
	α External::Loggers()ι->const vector<up<Logging::IExternalLogger>>&{ return _externalLoggers; }
	α External::Size()ι->uint{ return _externalLoggers.size(); }
	α External::Add( up<IExternalLogger>&& l )ι->void{
		l->Tags = TagSettings( Jde::format("logging/{}/tags", l->Name()), l->Name() );
		_externalLoggers.emplace_back( move(l) );
	}

	α External::Log( const MessageBase& m )ι->void{
		ASSERTX( m.Level!=ELogLevel::NoLog );
		for_each( _externalLoggers, [&](auto& x){ x->Log( m ); });
	}
	α External::Log( const MessageBase& m, const vector<string>& values )ι->void{
		ASSERTX( m.Level!=ELogLevel::NoLog );
		for_each( _externalLoggers, [&](auto& x){
			if( x->MinLevel()!=ELogLevel::NoLog && x->MinLevel()<=m.Level )
				x->Log( m, &values );
		});
	}
	α External::Log( const ExternalMessage& m )ι->void{
		ASSERTX( m.Level!=ELogLevel::NoLog );
		for_each( _externalLoggers, [&](auto& x){ x->Log( m ); });
	}

	α External::MinLevel()ι->ELogLevel{
		return std::accumulate( _externalLoggers.begin(), _externalLoggers.end(), ELogLevel::NoLog, [](ELogLevel min, auto& x){
			return Min( min, x->MinLevel() );
		});
	}
	α External::MinLevel( ELogTags tags )ι->ELogLevel{
		return std::accumulate( _externalLoggers.begin(), _externalLoggers.end(), ELogLevel::NoLog, [&](ELogLevel min, auto& x){
			return Min( min, Min(tags, x->Tags).value_or(x->DefaultLevel()) );
		});
	}
	α External::MinLevel( sv externalName )ι->ELogLevel{
		auto p = find_if( _externalLoggers, [externalName](auto& x){return x->Name()==externalName;} );
		if( p==_externalLoggers.end() )
			Trace( _tags, "Could not find external logger '{}'", externalName );
		return p==_externalLoggers.end() ? ELogLevel::NoLog : (*p)->MinLevel();
	}
	α External::DestroyLoggers()ι->void{
		for_each( _externalLoggers, [](auto& x){x->Destroy();} );
	}

	α External::SetMinLevel( ELogLevel external )ι->void{
		for_each( _externalLoggers, [&](auto& x){ x->SetMinLevel( external ); });
	}

}