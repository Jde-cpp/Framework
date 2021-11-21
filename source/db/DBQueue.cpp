#include "DBQueue.h"
#include "DataSource.h"
#include "Database.h"
#include "../threading/InterruptibleThread.h"

#define var const auto
namespace Jde::DB
{
	Statement::Statement( sv sql, const VectorPtr<object>& parameters, bool isStoredProc, SL sl ):
		Sql{ sql },
		Parameters{ parameters },
		IsStoredProc{isStoredProc},
		SourceLocation{ sl }
	{}

	DBQueue::DBQueue( sp<IDataSource> spDataSource )noexcept:
		_spDataSource{ spDataSource }
	{
		_pThread = ms<Threading::InterruptibleThread>( "DBQueue", [&](){Run();} );
		IApplication::AddThread( _pThread );
	}

	void DBQueue::Shutdown()noexcept
	{
		_pThread->Interrupt();
		while( !_stopped )
			std::this_thread::yield();
		//_queue.Push( sp<Statement>{} );
	}

	void DBQueue::Push( sv sql, const VectorPtr<object>& parameters, bool isStoredProc, SL sl )noexcept
	{
		RETURN_IF( _stopped, "pushing '{}' when stopped", sql );
		// if( !_stopped )
		// 	_queue.Push( ms<Statement>(sql, parameters, isStoredProc) );
		// else
		// 	DBG("pushing '{}' when stopped", sql);
		auto pStatement = ms<Statement>( sql, parameters, isStoredProc, sl );
		try
		{
			if( pStatement->IsStoredProc )
				_spDataSource->ExecuteProcNoLog( pStatement->Sql, *pStatement->Parameters, sl );
			else
				_spDataSource->ExecuteNoLog( pStatement->Sql, pStatement->Parameters.get(), nullptr, false, sl );
		}
		catch( const IException& e )
		{
			DB::LogNoServer( sql, parameters.get(), ELogLevel::Error, e.what(), sl );
		}
	}

	void DBQueue::Run()noexcept
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
		DBG( "DBQueue::Run - Ending"sv );
	}
}