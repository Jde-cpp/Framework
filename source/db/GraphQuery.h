#pragma once
#include <nlohmann/json.hpp>

namespace Jde::DB{ using nlohmann::json; struct TableQL; }
namespace Jde::DB::GraphQL
{
	α Query( const DB::TableQL& table, uint userId, json& jData )noexcept(false)->void;
}