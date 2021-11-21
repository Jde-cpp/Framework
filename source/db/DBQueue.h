﻿#pragma once
#include "DataType.h"
#include <atomic>
#include "../collections/Queue.h"
#include <jde/Exports.h>
#include <jde/App.h>

namespace Jde::Threading{struct InterruptibleThread;}
namespace Jde::DB
{
	struct Statement final
	{
		Statement( sv sql, const VectorPtr<object>& parameters, bool isStoredProc, SL sl );//TODO remove this.

		string Sql;//const?
		const VectorPtr<object> Parameters;
		bool IsStoredProc;//const?
		source_location SourceLocation;
	};

	struct IDataSource;

	struct Γ DBQueue final : IShutdown//, std::enable_shared_from_this<DBQueue>
	{
		DBQueue( sp<IDataSource> spDataSource )noexcept;
		α Push( sv statement, const VectorPtr<object>& parameters, bool isStoredProc=true, SRCE )noexcept->void;
		α Shutdown()noexcept->void override;
	private:
		α Run()noexcept->void;
		sp<Threading::InterruptibleThread> _pThread;
		Queue<Statement> _queue;
		sp<IDataSource> _spDataSource;
		std::atomic<bool> _stopped{false};
	};
}