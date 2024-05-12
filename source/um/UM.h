#pragma once
#ifndef UM_H
#define UM_H
#include <jde/Exports.h>
#include <jde/TypeDefs.h>
#include <jde/Exception.h>

namespace Jde::DB{ struct MutationQL; enum class EMutationQL : uint8; }

#define Φ Γ auto
namespace Jde::UM
{
	using ApiPK=uint;

	enum class EAccess : uint8{ None=0, Administer=1, Write=2, Read=4 };
	Ξ operator &( EAccess a, EAccess b ){ return static_cast<EAccess>( static_cast<uint8>(a) & static_cast<uint8>(b) ); }

	Φ Configure()ε->void;
	α IsTarget( sv url )ι->bool;
	α TestRead( str tableName, UserPK userId )ε->void;
	α TestAccess( EAccess access, UserPK userId, ApiPK apiId )ε->void;
	α ApplyMutation( const DB::MutationQL& m, UserPK id )ε->void;
	struct IAuthorize;
	Φ AddAuthorizer( UM::IAuthorize* p )ι->void;
	Φ FindAuthorizer( sv table )ι->IAuthorize*;
	struct IAuthorize{
		IAuthorize( sv table ):TableName{table}{ UM::AddAuthorizer( this ); }
		virtual ~IAuthorize()=0;
		β CanRead( uint /*pk*/, UserPK /*userId*/ )ι->bool { return true; }
		β CanPurge( uint /*pk*/, UserPK /*userId*/ )ι->bool{ return true; }
		α TestPurge( uint pk, UserPK userId, SRCE )ε->void;
		β Test( DB::EMutationQL ql, UserPK userId, SRCE )ε->void=0;
		//β Invalidate( SRCE )ε->void=0;
		sv TableName;
	};
	inline IAuthorize::~IAuthorize(){};
	struct GroupAuthorize : IAuthorize{
		GroupAuthorize():IAuthorize{"um_groups"}{}
		β CanPurge( uint pk, UserPK )ι->bool override{ return pk!=1 && pk!=2; };
		β Test( DB::EMutationQL, UserPK, SL )ε->void override{};//TODO Remove
		sv TableName;
	};
}
#undef Φ
#endif