#include <jde/io/File.h>

#include <cmath>
#include <fstream>
#include <streambuf>
#include <thread>
#include <unordered_set>
#include <spdlog/fmt/ostr.h>
#include "../Stopwatch.h"
#include <jde/Str.h>
#define var const auto

namespace Jde
{
	static var& _logLevel{ Logging::TagLevel("io") };
	α IO::FileSize( path path )noexcept(false)->uint
	{
		try
		{
			return fs::file_size( fs::canonical(path) );
		}
		catch( fs::filesystem_error& e )
		{
			throw IOException( move(e) );
		}
	}
	α IO::Copy( fs::path from, fs::path to, SL sl_ )->PoolAwait
	{
		return PoolAwait( [f=move(from), t=move(to), sl=sl_]()noexcept(false)
		{
			std::error_code e;
			fs::copy_file( f, t, e ); 
			THROW_IFX( e, IOException(fs::filesystem_error("copy file failed", f, t, e), sl) );
		}, sl_);
	}
	α IO::CopySync( fs::path from, fs::path to, SL sl )->void
	{
		VFuture( Copy(move(from), move(to), sl) ).get();
	}
	α IO::CreateDirectories( fs::path path )ε->void
	{
		try
		{
			fs::create_directories( path );
		}
		catch( fs::filesystem_error& e )
		{
			throw IOException( move(e) );
		}
	}
}
namespace Jde::IO::FileUtilities
{
	α ForEachItem( path path, std::function<void(const fs::directory_entry&)> function )noexcept(false)->void//__fs::filesystem::filesystem_error
	{
		for( var& dirEntry : fs::directory_iterator(path) )
			function( dirEntry );
	}

	up<std::set<fs::directory_entry>> GetDirectory( path directory )
	{
		auto items = mu<std::set<fs::directory_entry>>();
		Jde::IO::FileUtilities::ForEachItem( directory, [&items]( fs::directory_entry item ){items->emplace(item);} );
		return items;
	}

	up<std::set<fs::directory_entry>> GetDirectories( path directory, up<std::set<fs::directory_entry>> pItems )
	{
		if( !pItems )
			pItems = mu<std::set<fs::directory_entry>>();
		auto func = [&pItems]( fs::directory_entry item )
		{
			if( fs::is_directory(item) )
			{
				pItems->emplace( item );
				pItems = GetDirectories( item.path(), move(pItems) );
			}
		};
		Jde::IO::FileUtilities::ForEachItem( directory, func );
		return pItems;
	}

	up<vector<char>> LoadBinary( path path, SL sl )noexcept(false)//fs::filesystem_error
	{
		CHECK_PATH( path, sl );
		auto size = FileSize( path );
		TRACE( "Opening {} - {} bytes "sv, path.string(), size );
		std::ifstream f( path, std::ios::binary ); THROW_IFX( f.fail(), IOException(path, "Could not open file", sl) );

		return mu<vector<char>>( (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>() );  //vexing parse
	}
	string Load( path path, SL sl )noexcept(false)
	{
		CHECK_PATH( path, sl );
		auto size = FileSize( path );
		TRACE( "Opening {} - {} bytes "sv, path.string(), size );
		std::ifstream f( path, std::ios::binary ); THROW_IFX(f.fail(), IOException(path, "Could not open file") );
		string result;
		result.reserve( size );
		result.assign( (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>() );  //vexing parse
		return result;
	}
	α SaveBinary( path path, const std::vector<char>& data )noexcept(false)->void
	{
		std::ofstream f( path, std::ios::binary );
		THROW_IFX( f.fail(), IOException(path, "Could not open file") );

		f.write( data.data(), data.size() );
	}
	α Save( path path, sv value, std::ios_base::openmode /*openMode*/, SL sl )noexcept(false)->void
	{
		Save( path, ms<string>(value), sl );
	}
	α Save( fs::path path, sp<string> value, SL sl )noexcept(false)->void
	{
		VFuture( IO::Write(move(path), value, sl) ).get();
	}
	α Compression::Save( path path, const vector<char>& data )->void
	{
		SaveBinary( path, data );
		Compress( path );
	}
	up<vector<char>> Compression::LoadBinary( path uncompressed, path compressed, bool setPermissions, bool leaveUncompressed )noexcept(false)
	{
		var compressedPath = compressed.string().size() ? compressed : uncompressed;
		Extract( compressedPath );
		fs::path uncompressedFile = uncompressed;
		if( uncompressedFile.extension()==Extension() )
			uncompressedFile.replace_extension( "" );
		if( setPermissions )
			fs::permissions( uncompressedFile, fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read  );
		auto pResult = FileUtilities::LoadBinary( uncompressedFile );
		if( leaveUncompressed )
			fs::remove( compressedPath );
		else
			fs::remove( uncompressedFile );
		TRACE( "removed {}."sv, (leaveUncompressed ? compressedPath.string() : uncompressedFile.string()) );
		return pResult;
	}
	fs::path Compression::Compress( path path, bool deleteAfter )noexcept(false)
	{
		var command = CompressCommand( path );
		//LOGS( ELogLevel::Trace, command );
		THROW_IF( system(command.c_str())==-1, "{} failed.", command );
		if( deleteAfter && !CompressAutoDeletes() )
		{
			TRACE( "removed {}."sv, path.string() );
			VERIFY( fs::remove(path) );
		}
		auto compressedFile = path;
		return compressedFile.replace_extension( format("{}{}", path.extension().string(),Extension()) );
	}
	α Compression::Extract( path path )noexcept(false)->void
	{
		auto destination = string( path.parent_path().string() );
		fs::path compressedFile = path;
		if( compressedFile.extension()!=Extension() )
			compressedFile.replace_extension( format("{}{}", compressedFile.extension().string(),Extension()) );

		auto command = ExtractCommand( compressedFile, destination );// -y -bsp0 -bso0
		THROW_IFX( system(command.c_str())==-1, IO_EX(path, "{} failed.", command) );
	}
}
namespace Jde::IO
{
	uint File::Merge( path file, const vector<char>& original, const vector<char>& newData )noexcept(false)
	{
		//DBG( file.string() );
		std::fstream os{ file, std::ios::binary }; //CHECK( os );
		uint size = 0;
		for( uint i=0; i<std::min(original.size(), newData.size()); ++i )
		{
			if( original[i]==newData[i] ) continue;
			auto start = i;
			for( ; i<std::min(original.size(), newData.size()) && original[i]!=newData[i]; ++i );
			os.seekp( start );
			os.write( newData.data()+start, i-start ); //CHECK( os.good() );
			size+=i-start;
		}
		if( newData.size()>original.size() )
		{
			os.seekp( std::ios_base::end );
			os.write( newData.data()+original.size(), newData.size()-original.size() );
			size+=newData.size()-original.size();
			os.close();
		}
		else if( newData.size()<original.size() )
		{
			os.close();
			fs::resize_file( file, newData.size() );
			size+=original.size()-newData.size();
		}
		auto p = FileUtilities::LoadBinary( file );
		if( p->size()!=newData.size() )
			THROW( "error"sv );
		for( uint i=0; i<p->size(); ++i )
		{
			if( p->at(i)!=newData[i] )
				DBG( "{}"sv, i );
		}
		return size;
	}

	std::string FileUtilities::ToString( path filePath )
	{
		auto fileSize = FileSize( filePath );
		std::ifstream t( filePath );
		string str;
		str.reserve( fileSize );
		str.assign( (std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>() );
		return str;
	}

	std::string FileUtilities::DateFileName( uint16 year, uint8 month, uint8 day )noexcept
	{
		auto path = std::to_string(year);
		if( month>0 )
		{
			path = format( "{}-{:02d}", path, month );
			if( day>0 )
				path = format( "{}-{:02d}", path, day );
		}
		return path;
	}

	tuple<uint16,uint8,uint8> FileUtilities::ExtractDate( path path )noexcept
	{
		uint16 year=0;
		uint8 month=0, day=0;
		auto stem = path.stem().string();
		uint16 index = 0;
		for( ;index<stem.size() && !std::isdigit(stem[index]); ++index );
		stem = stem.substr( index );
		try
		{
			if( stem.size()>=4 )
			{
				year = stoul( stem.substr(0,4) );
				if( stem.size()>=7 && stem[4]=='-' )
				{
					month = (uint8)stoul( stem.substr(5,2) );
					if( stem.size()>=10 && stem[7]=='-' )
						day = (uint8)stoul( stem.substr(8,2) );
				}
			}
		}
		catch( const std::invalid_argument& )
		{
			DBG( "Could not convert '{}' to date."sv, path.string() );
		}
		return make_tuple( year, month, day );
	}
	α FileUtilities::Replace( path source, path destination, const flat_map<string,string>& replacements )noexcept(false)->void
	{
		auto sourceContent = Load( source );
		for( var& [search,replacement] : replacements )
			sourceContent = Str::Replace( sourceContent, search, replacement );
		Save( destination, sourceContent );
	}
}