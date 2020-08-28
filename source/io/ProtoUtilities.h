#pragma once

namespace Jde::IO::ProtoUtilities
{
	template<typename T>
	sp<T> Load( const fs::path& path, bool stupidPointer=false )noexcept(false);
	template<typename T>
	sp<T> TryLoad( const fs::path& path, bool stupidPointer=false )noexcept;
	template<typename T>
	vector<T> ToVector( const google::protobuf::RepeatedPtrField<T>& x )noexcept;

	void Save( const google::protobuf::MessageLite& msg, const fs::path& path )noexcept(false);
}


namespace Jde::IO
{
	inline void ProtoUtilities::Save( const google::protobuf::MessageLite& msg, const fs::path& path )noexcept(false)
	{
		string output;
		msg.SerializeToString( &output );
		FileUtilities::SaveBinary( path, output );
	}

	template<typename T>
	sp<T> ProtoUtilities::Load( const fs::path& path, bool stupidPointer )noexcept(false)
	{
		unique_ptr<vector<char>> pBytes;
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
		auto pValue = stupidPointer ? shared_ptr<T>( new T, [](T*){} ) : make_shared<T>();
		if( !pValue->MergePartialFromCodedStream(&input) )
		{
			if( stupidPointer )
				delete pValue.get();
			THROW( IOException(path, "MergePartialFromCodedStream returned false.") );
		}

		return pValue;
	}
	template<typename T>
	sp<T> ProtoUtilities::TryLoad( const fs::path& path, bool stupidPointer )noexcept
	{
		sp<T> pValue{};
		try
		{
			pValue = Load<T>( path, stupidPointer );
		}
		catch( Jde::Exception& e )
		{
			e.Log();
		}
		return pValue;
	}
	template<typename T>
	vector<T> ProtoUtilities::ToVector( const google::protobuf::RepeatedPtrField<T>& x )noexcept
	{
		vector<T> y;
		for_each( x.begin(), x.end(), [&y]( auto item ){ y.push_back(item); } );
		return y;
	}
}

