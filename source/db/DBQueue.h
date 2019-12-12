#pragma once
#include "DataType.h"
#include <atomic>
#include "../collections/Queue.h"
#include "../Exports.h"
#include "../application/Application.h"

namespace Jde::Threading{struct InterruptibleThread;}
namespace Jde::DB
{
	struct Statement
	{
		Statement( string_view sql, const VectorPtr<DB::DataValue>& parameters, bool isStoredProc );
		string Sql;
		const VectorPtr<DB::DataValue> Parameters;
		bool IsStoredProc;
	};

	struct IDataSource;

	struct JDE_NATIVE_VISIBILITY DBQueue : public IShutdown//, std::enable_shared_from_this<DBQueue>
	{
		DBQueue( sp<IDataSource> spDataSource )noexcept;
		void Push( string_view statement, const VectorPtr<DB::DataValue>& parameters, bool isStoredProc=true )noexcept;
		void Shutdown()noexcept override;
		//void Stop()noexcept{ DBG0("DBQueue::Stopping"); _stopped=true;}
	private:
		void Run()noexcept;
		sp<Threading::InterruptibleThread> _pThread;
		Queue<Statement> _queue;
		sp<IDataSource> _spDataSource;
		std::atomic<bool> _stopped{false};
	};
}