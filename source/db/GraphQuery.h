#pragma once
#include <jde/db/usings.h>

namespace Jde::DB{ struct TableQL; }
namespace Jde::DB::GraphQL{
	α Query( const DB::TableQL& table, json& jData, UserPK userId )ε->void;
}