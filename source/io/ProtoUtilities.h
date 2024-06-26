﻿#pragma once
#ifndef PROTO_UTILITIES_H
#define PROTO_UTILITIES_H
#pragma warning(push)
#pragma warning( disable : 4127 )
#pragma warning( disable : 5054 )
#include <google/protobuf/message.h>
#pragma warning(pop)
#include "proto/messages.pb.h"
#include <jde/io/File.h>

#define var const auto
namespace Jde::IO::Proto{
	Ŧ Load( const fs::path& path, SRCE )ε->up<T>;
	Ŧ TryLoad( const fs::path& path, SRCE )ι->up<T>;
	Ŧ Load( const fs::path& path, T& p, SRCE )ε->void;

	Ŧ Deserialize( const vector<char>& data )ε->up<T>;
	Ŧ Deserialize( const google::protobuf::uint8* p, int size )ε->T;
	Ŧ Deserialize( string&& x )ε->T;
	Ŧ ToVector( const google::protobuf::RepeatedPtrField<T>& x )ι->vector<T>;

	α Save( const google::protobuf::MessageLite& msg, fs::path path, SL )ε->void;
	α ToString( const google::protobuf::MessageLite& msg )ε->string;
	α SizePrefixed( const google::protobuf::MessageLite&& m )ι->tuple<up<google::protobuf::uint8[]>,uint>;
	α ToTimestamp( TimePoint t )ι->Logging::Proto::Timestamp;
	α ToTimePoint( Logging::Proto::Timestamp t )ι->TimePoint;
	
	namespace Internal{
		Ŧ Deserialize( const google::protobuf::uint8* p, int size, T& proto )ε->void{
			google::protobuf::io::CodedInputStream input{ p, (int)size };
			THROW_IF( !proto.MergePartialFromCodedStream(&input), "MergePartialFromCodedStream returned false." );
		}
	}
}


namespace Jde::IO
{
	Ξ Proto::ToString( const google::protobuf::MessageLite& msg )ε->string{
		string output;
		msg.SerializeToString( &output );
		return output;
	}

	Ξ Proto::SizePrefixed( const google::protobuf::MessageLite&& m )ι->tuple<up<google::protobuf::uint8[]>,uint>{
		const uint32_t length = (uint32_t)m.ByteSizeLong();
		var size = length+4;
		up<google::protobuf::uint8[]> pData{ new google::protobuf::uint8[size] };
		auto pDestination = pData.get();
		const char* pLength = reinterpret_cast<const char*>( &length )+3;
		for( auto i=0; i<4; ++i )
			*pDestination++ = *pLength--;
		var result = m.SerializeToArray( pDestination, (int)length ); LOG_IFT( !result, ELogLevel::Critical, IO::LogTag(), "Could not serialize to an array:{:x}", result );
		return make_tuple( move(pData), size );
	}
	Ξ Proto::Save( const google::protobuf::MessageLite& msg, fs::path path, SRCE )ε->void{
		var p = ms<string>();
		msg.SerializeToString( p.get() );
		FileUtilities::Save( move(path), p, sl );
	}

	Ŧ Proto::Deserialize( const vector<char>& data )ε->up<T>{
		auto p = mu<T>();
		Internal::Deserialize<T>( (google::protobuf::uint8*)data.data(), (uint32)data.size(), *p );
		return p;
	}
	Ŧ Proto::Deserialize( const google::protobuf::uint8* p, int size )ε->T{
		T y;
		Internal::Deserialize<T>( p, size, y );
		return y;
	}
	Ŧ Proto::Deserialize( string&& x )ε->T{
		T y;
		Internal::Deserialize<T>( (google::protobuf::uint8*)x.data(), (int)x.size(), y );
		x.clear();
		return y;
	}

	Ŧ Proto::Load( const fs::path& path, T& proto, SL sl )ε->void{
		up<vector<char>> pBytes;
		try{
			pBytes = IO::FileUtilities::LoadBinary( path, sl );
		}
		catch( fs::filesystem_error& e ){
			throw IOException( move(e) );
		}
		if( !pBytes || !pBytes->size() ){
			fs::remove( path );
			throw IOException{ path, "has 0 bytes. Removed", sl };
		}
		Internal::Deserialize( (google::protobuf::uint8*)pBytes->data(), (uint32)pBytes->size(), proto );
	}

	Ŧ Proto::Load( const fs::path& path, SL sl )ε->up<T>{
		auto p = mu<T>();
		Load( path, *p, sl );
		return p;
	}

	Ŧ Proto::TryLoad( const fs::path& path, SL sl )ι->up<T>{
		up<T> pValue{};
		if( fs::exists(path) ){
			try{
				pValue = Load<T>( path, sl );
			}
			catch( const IException& )
			{}
		}
		return pValue;
	}

	Ŧ Proto::ToVector( const google::protobuf::RepeatedPtrField<T>& x )ι->vector<T>{
		vector<T> y;
		for_each( x.begin(), x.end(), [&y]( auto item ){ y.push_back(item); } );
		return y;
	}

	Ξ Proto::ToTimestamp( TimePoint t )ι->Logging::Proto::Timestamp{
		Logging::Proto::Timestamp proto;
		var seconds = Clock::to_time_t( t );
		var nanos = std::chrono::duration_cast<std::chrono::nanoseconds>( t-TimePoint{} )-std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::seconds(seconds) );
		proto.set_seconds( seconds );
		proto.set_nanos( (int)nanos.count() );
		return proto;
	}
	Ξ Proto::ToTimePoint( Logging::Proto::Timestamp t )ι->TimePoint{
		Logging::Proto::Timestamp proto;
		return Clock::from_time_t( t.seconds() )+std::chrono::nanoseconds( t.nanos() );
	}
}
#undef var
#endif