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
	ⓣ Load( path path )noexcept(false)->up<T>;
	ⓣ TryLoad( path path )noexcept->up<T>;
	ⓣ Load( path path, T& p )noexcept(false)->void;
	ⓣ LoadXZ( path path )noexcept(false)->AWrapper;//sp<T>

	ⓣ Deserialize( const vector<char>& data )noexcept(false)->up<T>;
	ⓣ Deserialize( const google::protobuf::uint8* p, int size )noexcept(false)->T;

	ⓣ ToVector( const google::protobuf::RepeatedPtrField<T>& x )noexcept->vector<T>;

	α Save( const google::protobuf::MessageLite& msg, path path )noexcept(false)->void;
	α ToString( const google::protobuf::MessageLite& msg )noexcept(false)->string;
	α SizePrefixed( const google::protobuf::MessageLite& m )noexcept(false)->tuple<unique_ptr<google::protobuf::uint8[]>,uint>;
	α ToTimestamp( TimePoint t )->up<google::protobuf::Timestamp>;
	namespace Internal
	{
		ⓣ Deserialize( const google::protobuf::uint8* p, int size, T& proto )noexcept(false)->void
		{
			google::protobuf::io::CodedInputStream input{ p, (int)size };
			THROW_IF( !proto.MergePartialFromCodedStream(&input), "MergePartialFromCodedStream returned false." );
		}
	}
}


namespace Jde::IO
{
	inline string Proto::ToString( const google::protobuf::MessageLite& msg )noexcept(false)
	{
		string output;
		msg.SerializeToString( &output );
		return output;
	}

	inline tuple<unique_ptr<google::protobuf::uint8[]>,uint> Proto::SizePrefixed( const google::protobuf::MessageLite& m )noexcept(false)
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
	inline void Proto::Save( const google::protobuf::MessageLite& msg, path path )noexcept(false)
	{
		string output;
		msg.SerializeToString( &output );
		FileUtilities::SaveBinary( path, output );
	}

	ⓣ Proto::Deserialize( const vector<char>& data )noexcept(false)->up<T>
	{
		auto p = make_unique<T>();
		Internal::Deserialize<T>( (google::protobuf::uint8*)data.data(), (uint32)data.size(), *p );
		return p;
	}
	ⓣ Proto::Deserialize( const google::protobuf::uint8* p, int size )noexcept(false)->T
	{
		T y;
		Internal::Deserialize<T>( p, size, y );
		return y;
	}
	ⓣ Proto::Load( path path, T& proto )noexcept(false)->void
	{
		up<vector<char>> pBytes;
		try
		{
			pBytes = IO::FileUtilities::LoadBinary( path );
		}
		catch( fs::filesystem_error& e )
		{
			throw IOException( move(e) );
		}
		if( !pBytes )
		{
			fs::remove( path );
			throw IOException{ path, "has 0 bytes. Removed" };
		}
		Internal::Deserialize( (google::protobuf::uint8*)pBytes->data(), (uint32)pBytes->size(), proto );
	}

	ⓣ Proto::Load( path path )noexcept(false)->up<T>
	{
		auto p = make_unique<T>();
		Load( path, *p );
		return p;
	}

	ⓣ Proto::TryLoad( path path )noexcept->up<T>
	{
		up<T> pValue{};
		if( fs::exists(path) )
		{
			try
			{
				pValue = Load<T>( path );
			}
			catch( const IException& )
			{}
		}
		return pValue;
	}

	ⓣ Proto::ToVector( const google::protobuf::RepeatedPtrField<T>& x )noexcept->vector<T>
	{
		vector<T> y;
		for_each( x.begin(), x.end(), [&y]( auto item ){ y.push_back(item); } );
		return y;
	}

	inline α Proto::ToTimestamp( TimePoint t )->up<google::protobuf::Timestamp>
	{
		auto pTime = make_unique<google::protobuf::Timestamp>();
		var seconds = Clock::to_time_t( t );
		var nanos = std::chrono::duration_cast<std::chrono::nanoseconds>( t-TimePoint{} )-std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::seconds(seconds) );
		pTime->set_seconds( seconds );
		pTime->set_nanos( (int)nanos.count() );
		return pTime;
	}
}
#undef var
