#include "stdafx.h"
#include "DBQueue.h"
#include "DataSource.h"
#include "../threading/InterruptibleThread.h"

#define var const auto
namespace Jde::DB
{
	Statement::Statement( string_view sql, const VectorPtr<DB::DataValue>& parameters, bool isStoredProc ):
		Sql{ sql },
		Parameters{ parameters },
		IsStoredProc{isStoredProc}
	{}

	DBQueue::DBQueue( sp<IDataSource> spDataSource )noexcept:
		_spDataSource{ spDataSource }
	{
		_pThread = make_shared<Threading::InterruptibleThread>( "DBQueue", [&](){Run();} );
		IApplication::AddThread( _pThread );
	}

	void DBQueue::Shutdown()noexcept
	{
		_queue.Push( sp<Statement>{} );
	}

	void DBQueue::Push( string_view sql, const VectorPtr<DB::DataValue>& parameters, bool isStoredProc )noexcept
	{
		// if( !_stopped )
		// 	_queue.Push( make_shared<Statement>(sql, parameters, isStoredProc) );
		// else
		// 	DBG("pushing '{}' when stopped", sql);
		auto pStatement = make_shared<Statement>(sql, parameters, isStoredProc);
		try
		{
			if( pStatement->IsStoredProc )
				_spDataSource->ExecuteProc( pStatement->Sql, *pStatement->Parameters, false );
			else
				_spDataSource->Execute( pStatement->Sql, *pStatement->Parameters, false );
		}
		catch( const Exception& e )
		{
			ostringstream os;
			uint iParam = 0;
			for( char ch : pStatement->Sql )
			{
				if( ch=='?' )
				{
					const DB::DataValue& value = (*pStatement->Parameters)[iParam++];
					string value2 = to_string( value );
					os << value2;
				}
				else
					os << ch;
			}
			ERRX( "{} - {}", os.str(), e.what() );
		}
	}

	void DBQueue::Run()noexcept
	{
	//	Threading::SetThreadDescription( "DBQueue" );
		while( !Threading::GetThreadInterruptFlag().IsSet() || !_queue.Empty() )
		{
			var pStatement = _queue.WaitAndPop();
			if( !pStatement )
				continue;
			try
			{
				if( pStatement->IsStoredProc )
					_spDataSource->ExecuteProc( pStatement->Sql, *pStatement->Parameters, false );
				else
					_spDataSource->Execute( pStatement->Sql, *pStatement->Parameters, false );
			}
			catch( const Exception& e )
			{
				ostringstream os;
				uint iParam = 0;
				for( char ch : pStatement->Sql )
				{
					if( ch=='?' )
					{
						const DB::DataValue& value = (*pStatement->Parameters)[iParam++];
						os << to_string( value );
					}
					else
						os << ch;
				}
				ERRX( "{} - {}", os.str(), e.what() );
			}
		}
		_stopped = true;
		DBG0( "DBQueue::Run - Ending" );
	}
}