﻿#pragma once
#pragma warning(push)
#pragma warning( disable : 4127 )
#pragma warning( disable : 5054 )
#include <google/protobuf/message.h>
#include <google/protobuf/timestamp.pb.h>
#pragma warning(pop)

#include <jde/io/File.h>

#define var const auto
namespace Jde::IO::Proto
{
	Ŧ Load( path path, SRCE )noexcept(false)->up<T>;
	Ŧ TryLoad( path path, SRCE )noexcept->up<T>;
	Ŧ Load( path path, T& p, SRCE )noexcept(false)->void;

	Ŧ Deserialize( const vector<char>& data )noexcept(false)->up<T>;
	Ŧ Deserialize( const google::protobuf::uint8* p, int size )noexcept(false)->T;
	Ŧ Deserialize( string&& x )noexcept(false)->T;
	Ŧ ToVector( const google::protobuf::RepeatedPtrField<T>& x )noexcept->vector<T>;

	α Save( const google::protobuf::MessageLite& msg, fs::path path, SL )noexcept(false)->void;
	α ToString( const google::protobuf::MessageLite& msg )noexcept(false)->string;
	α SizePrefixed( const google::protobuf::MessageLite& m )noexcept(false)->tuple<up<google::protobuf::uint8[]>,uint>;
	α ToTimestamp( TimePoint t )->up<google::protobuf::Timestamp>;
	namespace Internal
	{
		Ŧ Deserialize( const google::protobuf::uint8* p, int size, T& proto )noexcept(false)->void
		{
			google::protobuf::io::CodedInputStream input{ p, (int)size };
			THROW_IF( !proto.MergePartialFromCodedStream(&input), "MergePartialFromCodedStream returned false." );
		}
	}
}


namespace Jde::IO
{
	Ξ Proto::ToString( const google::protobuf::MessageLite& msg )noexcept(false)->string
	{
		string output;
		msg.SerializeToString( &output );
		return output;
	}

	Ξ Proto::SizePrefixed( const google::protobuf::MessageLite& m )noexcept(false)->tuple<up<google::protobuf::uint8[]>,uint>
	{
		const uint32_t length = (uint32_t)m.ByteSizeLong();
		var size = length+4;
		up<google::protobuf::uint8[]> pData{ new google::protobuf::uint8[size] };
		auto pDestination = pData.get();
		const char* pLength = reinterpret_cast<const char*>( &length )+3;
		for( auto i=0; i<4; ++i )
			*pDestination++ = *pLength--;
		var result = m.SerializeToArray( pDestination, (int)length ); THROW_IF( !result, "Could not serialize to an array" );
		return make_tuple( move(pData), size );
	}
	Ξ Proto::Save( const google::protobuf::MessageLite& msg, fs::path path, SRCE )noexcept(false)->void
	{
		var p = ms<string>();
		msg.SerializeToString( p.get() );
		FileUtilities::Save( move(path), p, sl );
	}

	Ŧ Proto::Deserialize( const vector<char>& data )noexcept(false)->up<T>
	{
		auto p = mu<T>();
		Internal::Deserialize<T>( (google::protobuf::uint8*)data.data(), (uint32)data.size(), *p );
		return p;
	}
	Ŧ Proto::Deserialize( const google::protobuf::uint8* p, int size )noexcept(false)->T
	{
		T y;
		Internal::Deserialize<T>( p, size, y );
		return y;
	}
	Ŧ Proto::Deserialize( string&& x )noexcept(false)->T
	{
		T y;
		Internal::Deserialize<T>( (google::protobuf::uint8*)x.data(), (int)x.size(), y );
		x.clear();
		return y;
	}

	Ŧ Proto::Load( path path, T& proto, SL sl )noexcept(false)->void
	{
		up<vector<char>> pBytes;
		try
		{
			pBytes = IO::FileUtilities::LoadBinary( path, sl );
		}
		catch( fs::filesystem_error& e )
		{
			throw IOException( move(e) );
		}
		if( !pBytes || !pBytes->size() )
		{
			fs::remove( path );
			throw IOException{ path, "has 0 bytes. Removed", sl };
		}
		Internal::Deserialize( (google::protobuf::uint8*)pBytes->data(), (uint32)pBytes->size(), proto );
	}

	Ŧ Proto::Load( path path, SL sl )noexcept(false)->up<T>
	{
		auto p = mu<T>();
		Load( path, *p, sl );
		return p;
	}

	Ŧ Proto::TryLoad( path path, SL sl )noexcept->up<T>
	{
		up<T> pValue{};
		if( fs::exists(path) )
		{
			try
			{
				pValue = Load<T>( path, sl );
			}
			catch( const IException& )
			{}
		}
		return pValue;
	}

	Ŧ Proto::ToVector( const google::protobuf::RepeatedPtrField<T>& x )noexcept->vector<T>
	{
		vector<T> y;
		for_each( x.begin(), x.end(), [&y]( auto item ){ y.push_back(item); } );
		return y;
	}

	Ξ Proto::ToTimestamp( TimePoint t )->up<google::protobuf::Timestamp>
	{
		auto pTime = mu<google::protobuf::Timestamp>();
		var seconds = Clock::to_time_t( t );
		var nanos = std::chrono::duration_cast<std::chrono::nanoseconds>( t-TimePoint{} )-std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::seconds(seconds) );
		pTime->set_seconds( seconds );
		pTime->set_nanos( (int)nanos.count() );
		return pTime;
	}
}
#undef var