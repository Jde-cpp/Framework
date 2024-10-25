#include <jde/framework/io/file.h>

#include <cmath>
#include <fstream>
#include <streambuf>
#include <thread>
#include <unordered_set>
#include <spdlog/fmt/ostr.h>
#include "../Stopwatch.h"
#include <jde/framework/str.h>
#define let const auto

namespace Jde{
	constexpr ELogTags _tags{ ELogTags::IO };
//	α IO::LogTag()ι->sp<Jde::LogTag>{ return _logTag; }

	α IO::FileSize( const fs::path& path )ε->uint{
		try{
			return fs::file_size( fs::canonical(path) );
		}
		catch( fs::filesystem_error& e ){
			throw IOException( move(e) );
		}
	}
	α IO::Remove( const fs::path& p, SL sl )ε->void
	{
		std::error_code e;
		fs::remove( p, e );
		THROW_IFX( e, IOException(fs::filesystem_error("remove file failed", p, e), sl) );
		Trace{ sl, _tags, "Removed {}", p.string() };
	}
	α IO::Copy( fs::path from, fs::path to, SL sl_ )->PoolAwait
	{
		return PoolAwait( [f=move(from), t=move(to), sl=sl_]()ε
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
	α ForEachItem( const fs::path& path, std::function<void(const fs::directory_entry&)> function )ε->void//__fs::filesystem::filesystem_error
	{
		for( let& dirEntry : fs::directory_iterator(path) )
			function( dirEntry );
	}

	up<std::set<fs::directory_entry>> GetDirectory( const fs::path& directory )
	{
		auto items = mu<std::set<fs::directory_entry>>();
		Jde::IO::FileUtilities::ForEachItem( directory, [&items]( fs::directory_entry item ){items->emplace(item);} );
		return items;
	}

	up<std::set<fs::directory_entry>> GetDirectories( const fs::path& directory, up<std::set<fs::directory_entry>> pItems )
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

	α LoadBinary( const fs::path& path, SL sl )ε->up<vector<char>>{//fs::filesystem_error
		CHECK_PATH( path, sl );
		auto size = FileSize( path );
		Trace{ sl, _tags, "Opening {} - {} bytes ", path.string(), size };
		std::ifstream f( path, std::ios::binary ); THROW_IFX( f.fail(), IOException(path, "Could not open file", sl) );

		return mu<vector<char>>( (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>() );  //vexing parse
	}
	α Load( const fs::path& path, SL sl )ε->string{
		CHECK_PATH( path, sl );
		auto size = FileSize( path );
		Trace{ sl, _tags, "Opening {} - {} bytes ", path.string(), size };
		std::ifstream f( path, std::ios::binary ); THROW_IFX(f.fail(), IOException(path, "Could not open file") );
		string result;
		result.reserve( size );
		result.assign( (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>() );  //vexing parse
		return result;
	}
	α SaveBinary( const fs::path& path, const std::vector<char>& data )ε->void
	{
		std::ofstream f( path, std::ios::binary );
		THROW_IFX( f.fail(), IOException(path, "Could not open file") );

		f.write( data.data(), data.size() );
	}
	α Save( const fs::path& path, sv value, std::ios_base::openmode /*openMode*/, SL sl )ε->void
	{
		Save( path, ms<string>(value), sl );
	}
	α Save( fs::path path, sp<string> value, SL sl )ε->void
	{
		VFuture( IO::Write(move(path), value, sl) ).get();
	}
/*	α Compression::Save( const fs::path& path, const vector<char>& data )->void
	{
		SaveBinary( path, data );
		Compress( path );
	}
	up<vector<char>> Compression::LoadBinary( const fs::path& uncompressed, const fs::path& compressed, bool setPermissions, bool leaveUncompressed )ε
	{
		let compressedPath = compressed.string().size() ? compressed : uncompressed;
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
		TRACE( "removed {}.", (leaveUncompressed ? compressedPath.string() : uncompressedFile.string()) );
		return pResult;
	}
	fs::path Compression::Compress( const fs::path& path, bool deleteAfter )ε
	{
		let command = CompressCommand( path );
		THROW_IF( system(command.c_str())==-1, "{} failed.", command );
		if( deleteAfter && !CompressAutoDeletes() )
		{
			TRACE( "removed {}.", path.string() );
			VERIFY( fs::remove(path) );
		}
		auto compressedFile = path;
		return compressedFile.replace_extension( Jde::format("{}{}", path.extension().string(),Extension()) );
	}
	α Compression::Extract( const fs::path& path )ε->void
	{
		auto destination = string( path.parent_path().string() );
		fs::path compressedFile = path;
		if( compressedFile.extension()!=Extension() )
			compressedFile.replace_extension( Jde::format("{}{}", compressedFile.extension().string(),Extension()) );

		auto command = ExtractCommand( compressedFile, destination );// -y -bsp0 -bso0
		THROW_IFX( system(command.c_str())==-1, IO_EX(path, ELogLevel::Error, "{} failed.", command) );
	}
*/
}
namespace Jde::IO{
	uint File::Merge( const fs::path& file, const vector<char>& original, const vector<char>& newData )ε{
		std::fstream os{ file, std::ios::binary }; //CHECK( os );
		uint size = 0;
		for( uint i=0; i<std::min(original.size(), newData.size()); ++i ){
			if( original[i]==newData[i] ) continue;
			auto start = i;
			for( ; i<std::min(original.size(), newData.size()) && original[i]!=newData[i]; ++i );
			os.seekp( start );
			os.write( newData.data()+start, i-start ); //CHECK( os.good() );
			size+=i-start;
		}
		if( newData.size()>original.size() ){
			os.seekp( std::ios_base::end );
			os.write( newData.data()+original.size(), newData.size()-original.size() );
			size+=newData.size()-original.size();
			os.close();
		}
		else if( newData.size()<original.size() ){
			os.close();
			fs::resize_file( file, newData.size() );
			size+=original.size()-newData.size();
		}
		auto p = FileUtilities::LoadBinary( file );
		if( p->size()!=newData.size() )
			THROW( "error" );
		for( uint i=0; i<p->size(); ++i ){
			if( p->at(i)!=newData[i] )
				Trace{ _tags, "{}", i };
		}
		return size;
	}

	std::string FileUtilities::ToString( const fs::path& filePath )
	{
		auto fileSize = FileSize( filePath );
		std::ifstream t( filePath );
		string str;
		str.reserve( fileSize );
		str.assign( (std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>() );
		return str;
	}

	std::string FileUtilities::DateFileName( uint16 year, uint8 month, uint8 day )ι
	{
		auto path = std::to_string(year);
		if( month>0 )
		{
			path = Jde::format( "{}-{:02d}", path, month );
			if( day>0 )
				path = Jde::format( "{}-{:02d}", path, day );
		}
		return path;
	}

	tuple<uint16,uint8,uint8> FileUtilities::ExtractDate( const fs::path& path )ι
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
		catch( const std::invalid_argument& ){
			Warning{ _tags, "Could not convert '{}' to date.", path.string() };
		}
		return make_tuple( year, month, day );
	}
	α FileUtilities::Replace( const fs::path& source, const fs::path& destination, const flat_map<string,string>& replacements )ε->void
	{
		auto sourceContent = Load( source );
		for( let& [search,replacement] : replacements )
			sourceContent = Str::Replace( sourceContent, search, replacement );
		Save( destination, sourceContent );
	}
}
namespace Jde
{
	α IO::ForEachLine( const fs::path& path, function<void(const vector<double>&, uint lineIndex)> function, const flat_set<uint>& columnIndexes_, const uint maxLines/*=std::numeric_limits<uint>::max()*/, const uint startLine/*=0*/, const uint /*chunkSize=1073741824*/, uint maxColumnCount/*=1500*/, Stopwatch* /*sw=nullptr*/, double emptyValue/*=0.0*/ )->uint
	{
		let pBuffer = FileUtilities::LoadBinary( path );
		let& buffer = *pBuffer;
		vector<uint_fast8_t> columnIndexes;
		for( uint i=0, index=0; columnIndexes_.size() && i<maxColumnCount; ++i )
			columnIndexes.push_back( columnIndexes_.find(index++)!=columnIndexes_.end() ? uint_fast8_t(1) : uint_fast8_t(0) );

		const char* p = buffer.data(), *pEnd = p+buffer.size();
		uint i = 0;
		for( ; i<startLine && p<pEnd; ++i, ++p )
			p = strchr( p, '\n' );

		vector<double> tokens;
		for( ; p<pEnd && maxLines>i-startLine; ++i )
		{
			tokens.clear();
			auto pStart = p;
			for( uint j=0; p<pEnd && (p==pStart || (*(p-1)!='\r' && *(p-1)!='\n')); ++p, ++j )
			{
				if( !columnIndexes_.empty() && !columnIndexes[j] )
					for( ; *p!=',' && *p!='\n' && *p!='\r'; ++p );
				else
				{
					let empty = *p==',' || *p=='\n';
					double value = empty ? emptyValue : 0.0;
					if( !empty )
					{
						const bool negative = *p == '-';
						if( negative )
							++p;
						for( ;*p >= '0' && *p <= '9'; ++p )
							value = value*10.0 + *p - '0';
						if (*p == '.')
						{
							++p;
							int n = 0;
							double f = 0.0;
							for( ;*p >= '0' && *p <= '9'; ++p,++n )
								f = f*10.0 + *p - '0';
							value += f / std::pow(10.0, n);
						}
						if( *p=='e' )//2.31445e+06
						{
							++p;
							bool positiveExponent = *p=='+';
							++p;
							_int exponent = 0;
							for( ;*p >= '0' && *p <= '9'; ++p )
								exponent = exponent*10 + *p - '0';
							if( !positiveExponent )
								exponent*=-1;
							value = value*double( pow(10,exponent) );
						}
						if( negative )
							value = -value;
					}
					tokens.push_back( value );
				}
			}
			function( tokens, i );
		}
		return i-startLine;
	}

	α IO::LoadColumnNames( const fs::path& path, const vector<string>& columnNamesToFetch, bool notColumns )->tuple<vector<string>,flat_set<uint>>
	{
		vector<string> columnNames;
		flat_set<uint> columnIndexes;
		std::ifstream f( path ); THROW_IFX( f.fail(), IOException(path, "Could not open file") );
		string line;
		if( std::getline<char>(f, line) )
		{
			auto tokens = Str::Split( line );
			int iToken=0;
			for( let& token : tokens )
			{
				if( (!notColumns && std::find( columnNamesToFetch.begin(), columnNamesToFetch.end(), token)!=columnNamesToFetch.end())
					|| (notColumns && std::find( columnNamesToFetch.begin(), columnNamesToFetch.end(), token)==columnNamesToFetch.end()) )
				{
					columnNames.push_back( string{token} );
					columnIndexes.insert( iToken );
				}
				++iToken;
			}
		};

		return make_tuple( columnNames, columnIndexes );
	}
}