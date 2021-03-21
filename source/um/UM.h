#pragma once
#include "../TypeDefs.h"

namespace Jde
{
	namespace DB{ struct MutationQL; }
	typedef uint32 UserPK;
}

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
	inline EAccess operator &( EAccess a, EAccess b ){ return static_cast<EAccess>( static_cast<uint8>(a) & static_cast<uint8>(b) ); }
	//struct Service //: IService
	//{
		//static sp<Service> Instance()noexcept{ return _pInstance.get(); }
		void Configure()noexcept(false);
		bool IsTarget( sv url )noexcept;
		void TestAccess( EAccess access, UserPK userId, sv tableName )noexcept(false);
		void TestAccess( EAccess access, UserPK userId, ApiPK apiId )noexcept(false);
		void ApplyMutation( const DB::MutationQL& m, PK id )noexcept(false);
		// string Get( sv url, UserPK userId )noexcept(false);
		// UserPK Post( sv url, UserPK userId, sv item )noexcept(false);
		// void Patch( sv url, UserPK userId, sv item )noexcept(false);
		// void Delete( sv url, UserPK userId )noexcept(false);
//	};
}