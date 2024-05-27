#pragma once
#include "../GraphQL.h"
#include "../../coroutine/Awaitable.h"
#include "../types/Table.h"

namespace Jde::DB::GraphQL{
	struct PurgeAwait final: AsyncAwait{
		PurgeAwait( const DB::Table& table, const DB::MutationQL& mutation, uint extendedFromId, SRCE )ι;
		α Execute( const DB::Table& table, DB::MutationQL m, UserPK userId, HCoroutine h )ι->Task;
	};
}