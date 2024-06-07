#pragma once
#include "../GraphQL.h"
#include "../../coroutine/Awaitable.h"
#include "../types/Table.h"

namespace Jde::DB::GraphQL{
	struct InsertAwait final: AsyncReadyAwait{
		InsertAwait( const DB::Table& table, const DB::MutationQL& mutation, UserPK userPK, uint extendedFromId, SRCE )ι;
		α CreateQuery( const DB::Table& table, uint extendedFromId )ι->optional<AwaitResult>;
		α Execute( HCoroutine h, UserPK userPK )ι->Task;
	private:
		const DB::MutationQL _mutation;
		vector<DB::object> _parameters;
		ostringstream _sql;
	};
}