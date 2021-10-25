#pragma once
#include <jde/Exports.h>
#include <jde/TypeDefs.h>

namespace Jde::DB{ struct MutationQL; }

namespace Jde::UM
{
	typedef uint ApiPK;

	enum class EAccess : uint8
	{
		None=0,
		Administer=1,
		Write=2,
		Read=4
	};
	inline α operator &( EAccess a, EAccess b ){ return static_cast<EAccess>( static_cast<uint8>(a) & static_cast<uint8>(b) ); }

	Γ α Configure()noexcept(false)->void;
	α IsTarget( sv url )noexcept->bool;
	α TestAccess( EAccess access, UserPK userId, sv tableName )noexcept(false)->void;
	α TestAccess( EAccess access, UserPK userId, ApiPK apiId )noexcept(false)->void;
	α ApplyMutation( const DB::MutationQL& m, PK id )noexcept(false)->void;
}