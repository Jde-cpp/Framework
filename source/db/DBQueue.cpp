﻿#include "DBQueue.h"
#include "DataSource.h"
#include "Database.h"
#include "../threading/InterruptibleThread.h"

#define var const auto
namespace Jde::DB{
	static sp<LogTag> _logTag{ Logging::Tag("sql") };
	Statement::Statement( string sql, sp<vector<object>> parameters, bool isStoredProc, SL sl ):
		Sql{ move(sql) },
		Parameters{ parameters },
		IsStoredProc{isStoredProc},
		SourceLocation{ sl }
	{}

	DBQueue::DBQueue( sp<IDataSource> spDataSource )ι:
		_spDataSource{ spDataSource }{
		_pThread = ms<Threading::InterruptibleThread>( "DBQueue", [&](){Run();} );
		IApplication::AddThread( _pThread );
	}

	void DBQueue::Shutdown( bool /*terminate*/ )ι{
		_pThread->Interrupt();
		while( !_stopped )
			std::this_thread::yield();
		//_queue.Push( sp<Statement>{} );
	}

	void DBQueue::Push( string sql, sp<vector<object>> parameters, bool isStoredProc, SL sl )ι{
		if( _stopped )
			return;
		// if( !_stopped )
		// 	_queue.Push( ms<Statement>(sql, parameters, isStoredProc) );
		// else
		// 	TRACE("pushing '{}' when stopped", sql);
		auto pStatement = ms<Statement>( move(sql), parameters, isStoredProc, sl );
		try
		{
			if( pStatement->IsStoredProc )
				_spDataSource->ExecuteProcNoLog( move(pStatement->Sql), *pStatement->Parameters, sl );
			else
				_spDataSource->ExecuteNoLog( move(pStatement->Sql), pStatement->Parameters.get(), nullptr, false, sl );
		}
		catch( const IException& )
		{
			//DB::LogNoServer( move(pStatement->Sql), parameters.get(), ELogLevel::Error, e.what(), sl );
		}
	}

	void DBQueue::Run()ι
	{
	//	Threading::SetThreadDescription( "DBQueue" );
		while( !Threading::GetThreadInterruptFlag().IsSet() || !_queue.Empty() )
		{
			var pStatement = _queue.WaitAndPop( 1s );
			if( !pStatement )
				continue;
			try
			{
				if( pStatement->IsStoredProc )
					_spDataSource->ExecuteProcNoLog( pStatement->Sql, *pStatement->Parameters );
				else
					_spDataSource->ExecuteNoLog( pStatement->Sql, pStatement->Parameters.get() );
			}
			catch( const IException& e )
			{
				DB::LogNoServer( pStatement->Sql, pStatement->Parameters.get(), ELogLevel::Error, e.what(), pStatement->SourceLocation );
			}
		}
		_stopped = true;
		_spDataSource = nullptr;
		TRACE( "DBQueue::Run - Ending"sv );
	}
}