#pragma once
#include <jde/Exports.h>
#include <jde/TypeDefs.h>
#include <jde/Exception.h>

namespace Jde::DB{ struct MutationQL; }

#define Φ Γ auto
namespace Jde::UM
{
	using ApiPK=uint;

	enum class EAccess : uint8{ None=0, Administer=1, Write=2, Read=4 };
	Ξ operator &( EAccess a, EAccess b ){ return static_cast<EAccess>( static_cast<uint8>(a) & static_cast<uint8>(b) ); }

	Φ Configure()noexcept(false)->void;
	α IsTarget( sv url )noexcept->bool;
	α TestRead( str tableName, UserPK userId )noexcept(false)->void;
	//α TestAccess( EAccess access, UserPK userId, str tableName )noexcept(false)->void;
	α TestAccess( EAccess access, UserPK userId, ApiPK apiId )noexcept(false)->void;
	α ApplyMutation( const DB::MutationQL& m, PK id )noexcept(false)->void;
	struct IAuthorize;
	Φ AddAuthorizer( UM::IAuthorize* p )noexcept->void;
	Φ FindAuthorizer( sv table )noexcept->IAuthorize*;
	struct IAuthorize
	{
		IAuthorize( sv table ):TableName{table}{ UM::AddAuthorizer( this ); }
		virtual ~IAuthorize()=0;
		//β TestAccess( EAccess access, UserPK userId )noexcept(false)->void=0;
		β CanRead( uint pk, UserPK userId )noexcept->bool{ return true; }
		β CanPurge( uint pk, UserPK userId )noexcept->bool{ return true; }
		α TestPurge( uint pk, UserPK userId, SRCE )noexcept(false)->void;
//		β SetInstance( IAuthorize* _pInstance )noexcept->void=0;
		//β GetInstance()noexcept->IAuthorize* = 0;
		sv TableName;
	private:
		//const LogTag& _logLevel;
	};
	inline IAuthorize::~IAuthorize(){};
	struct GroupAuthorize : IAuthorize
	{
		GroupAuthorize():IAuthorize{"um_groups"}{}
		//β TestAccess( EAccess access, UserPK userId )noexcept(false)->void=0;
		//β CanRead( uint pk, UserPK userId )noexcept->bool{ return true; }
		β CanPurge( uint pk, UserPK )noexcept->bool override{ return pk!=1 && pk!=2; };
//		β SetInstance( IAuthorize* _pInstance )noexcept->void=0;
		//β GetInstance()noexcept->IAuthorize* = 0;
		sv TableName;
	};
}
#undef Φ