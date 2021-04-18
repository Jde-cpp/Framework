#pragma once
#include "File.h"

namespace Jde::IO::Proto
{
	template<typename T>
	up<T> Load( path path )noexcept(false);
	template<typename T>
	up<T> TryLoad( path path )noexcept;
	template<typename T>
	void Load( path path, T& p )noexcept(false);

	template<typename T>
	vector<T> ToVector( const google::protobuf::RepeatedPtrField<T>& x )noexcept;

	void Save( const google::protobuf::MessageLite& msg, path path )noexcept(false);
	string ToString( const google::protobuf::MessageLite& msg )noexcept(false);
}


namespace Jde::IO
{
	inline string Proto::ToString( const google::protobuf::MessageLite& msg )noexcept(false)
	{
		string output;
		msg.SerializeToString( &output );
		return output;
	}

	inline void Proto::Save( const google::protobuf::MessageLite& msg, path path )noexcept(false)
	{
		string output;
		msg.SerializeToString( &output );
		FileUtilities::SaveBinary( path, output );
	}

	template<typename T>
	void Proto::Load( path path, T& proto )noexcept(false)
	{
		up<vector<char>> pBytes;
		try
		{
			pBytes = IO::FileUtilities::LoadBinary( path );
		}
		catch( const fs::filesystem_error& e )
		{
			THROW( IOException(e) );
		}
		if( !pBytes )
		{
			fs::remove( path );
			THROW( IOException(path, "has 0 bytes. Removed") );
		}

		google::protobuf::io::CodedInputStream input{ (const uint8*)pBytes->data(), (int)pBytes->size() };
		if( !proto.MergePartialFromCodedStream(&input) )
			THROW( IOException(path, "MergePartialFromCodedStream returned false.") );
	}

	template<typename T>
	up<T> Proto::Load( path path )noexcept(false)
	{
		auto p = make_unique<T>();
		Load( path, *p );
		return p;
	}
	template<typename T>
	up<T> Proto::TryLoad( path path )noexcept
	{
		up<T> pValue{};
		try
		{
			pValue = Load<T>( path );
		}
		catch( Jde::Exception& e )
		{
			e.Log();
		}
		return pValue;
	}
	template<typename T>
	vector<T> Proto::ToVector( const google::protobuf::RepeatedPtrField<T>& x )noexcept
	{
		vector<T> y;
		for_each( x.begin(), x.end(), [&y]( auto item ){ y.push_back(item); } );
		return y;
	}
}

