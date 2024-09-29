#pragma once
#include <jde/db/usings.h>
#include "DataType.h"

namespace Jde::DB{ struct TableQL; }
namespace Jde::DB::GraphQL{
	α Query( const DB::TableQL& table, json& jData, UserPK userId )ε->void;
	Γ α SelectStatement( const DB::TableQL& table, bool includeIdColumn, string* whereString=nullptr )ι->tuple<string,vector<DB::object>>;
}