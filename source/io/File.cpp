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
		auto items = make_unique<std::set<fs::directory_entry>>();
		Jde::IO::FileUtilities::ForEachItem( directory, [&items]( fs::directory_entry item ){items->emplace(item);} );
		return items;
	}

	up<std::set<fs::directory_entry>> GetDirectories( path directory, up<std::set<fs::directory_entry>> pItems )
	{
		if( !pItems )
			pItems = make_unique<std::set<fs::directory_entry>>();
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

	up<vector<char>> LoadBinary( path path )noexcept(false)//fs::filesystem_error
	{
		CHECK_PATH( path );
		auto size = FileSize( path );
		TRACE( "Opening {} - {} bytes "sv, path.string(), size );
		std::ifstream f( path, std::ios::binary ); THROW_IFX( f.fail(), IOException(path, "Could not open file") );

		return make_unique<vector<char>>( (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>() );  //vexing parse
	}
	string Load( path path )noexcept(false)
	{
		CHECK_PATH( path );
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
		// if( !fs::exists(path.parent_path()) )
		// {
		// 	fs::create_directories( path.parent_path() );
		// 	INFO( "Created directory '{}'", path.parent_path() );
		// }
		// std::ofstream f{ path, openMode }; THROW_IFX( f.fail(), IOException(path, "Could not open file", sl) );
		// f.write( value.data(), value.size() );
	}
	α Save( path path, sp<string> value, SL sl )noexcept(false)->void
	{
		Future<sp<void>>( IO::Write(path, value, sl) ).get();
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

	vector<string> FileUtilities::LoadColumnNames( path csvFileName )
	{
		std::vector<string> columnNames;
		auto columnNameFunction = [&columnNames](string line){ columnNames = Str::Split<char>(line); };
		IO::File::ForEachLine<char>( csvFileName.string(), columnNameFunction, 1 );
		return columnNames;
	}

	α File::ForEachLine( sv filePath, const std::function<void(sv)>& function, const size_t lineCount )->void
	{
		std::ifstream file( string(filePath).c_str() ); THROW_IFX(file.fail(), IOException(filePath, "Could not open file") );
		//String line;
		std::string line;

		for( size_t index = 0; index<lineCount && getline<char>(file, line); ++index )
			function( line );
	}
	size_t File::ForEachLine( sv pszFileName, const std::function<void(const std::vector<string>&, size_t lineIndex)>& function, const std::set<size_t>& columnIndexes, const size_t maxLines/*=std::numeric_limits<size_t>::max()*/, const size_t startLine/*=0*/, const size_t /*chunkSize=1073741824*/, size_t maxColumnCount/*=1500*/ )
	{
		std::ifstream file2( string(pszFileName).c_str(), std::ios::binary | std::ios::ate );
		const size_t fileSize = file2.tellg();
		file2.close();
		std::ifstream file( string(pszFileName).c_str() ); THROW_IFX( file.fail(), IOException(pszFileName, "Could not open file") );
		size_t chunkSize2=fileSize;
		std::vector<char> rgBuffer( chunkSize2 );
		vector<string> tokens;
		size_t lineIndex = 0;
		vector<uint_fast8_t> columnIndexes2; columnIndexes2.reserve( maxColumnCount );
		size_t found=0;
		size_t index=0;
		for( size_t i=0; i<maxColumnCount; ++i )
		{
			bool exists = columnIndexes.find(index++)!=columnIndexes.end();
			columnIndexes2.push_back( exists ? uint_fast8_t(1) : uint_fast8_t(0) );
			if( exists )
				++found;
		}
		uint_fast8_t* pColumnIndexes2 = columnIndexes2.data();

		size_t tokenIndex=0;
		long columnIndex=0;
		//TODO:  Make so it goes through multiple reads
		do
		{
			file.read( rgBuffer.data(), size_t(chunkSize2)/**/ );

			const size_t charactersRead = file.gcount();
			char* pStart = rgBuffer.data(); auto pEnd = pStart+charactersRead;
			char* p = pStart;
			if( lineIndex==0 && startLine>0 )
			{
				for( ; lineIndex<startLine; ++lineIndex )
					p = strchr( &rgBuffer[0], '\n' );
				++p;
			}

			for( ; p<pEnd; ++p, ++columnIndex )
			{
				const uint_fast8_t getToken = pColumnIndexes2[columnIndex];
				if( !getToken )
				{
					for( ; *p!=',' && *p!='\n'&& *p!='\r'; ++p );
					const bool newLineStart = *p=='\r' || *p=='\n' || ( p==p+charactersRead-1 && file.eof() );
					if( !newLineStart )
						continue;
				}
				else
				{
					string value;
					for( ; *p!=',' && *p!='\n'&& *p!='\r'; ++p )
						value+=*p;
					if( tokens.size()<=tokenIndex )
						tokens.push_back( value );
					else
						tokens[tokenIndex] = value;
					++tokenIndex;
					const bool newLineStart = *p=='\n' || *p=='\r' || ( p==p+charactersRead-1 && file.eof() );
					if( !newLineStart )
						continue;
				}
				while( tokens.size()>tokenIndex )
					tokens.pop_back();
				function( tokens, lineIndex );
				columnIndex=-1;
				tokenIndex=0;
				if( ++lineIndex-startLine>=maxLines )
					break;
			}
			if( lineIndex-startLine>=maxLines )
				break;
		}while( !file.eof() );
		return lineIndex-startLine;
	}

	size_t File::ForEachLine2( path path, const std::function<void(const std::vector<std::string>&, size_t lineIndex)>& function, const std::set<size_t>& columnIndexes, const size_t lineCount/*=std::numeric_limits<size_t>::max()*/, const size_t startLine, const size_t chunkSize/*=2^30*/, Stopwatch* /* pStopwatch*/ )noexcept(false)
	{
		//FILE* pFile = fopen( file.c_str(), "r" );
		std::ifstream file( path.string() ); THROW_IFX( file.fail(), IOException(path, "Could not open file") );

		std::vector<char> rgBuffer( chunkSize );
		vector<string> tokens;

		size_t lineIndex = 0;
		vector<uint_fast8_t> columnIndexes3;
		size_t found=0;

		size_t index=0;
		for( int i=0; i<1500; ++i )
		{
			bool exists = columnIndexes.find(index++)!=columnIndexes.end();
			columnIndexes3.push_back( exists ? uint_fast8_t(1) : uint_fast8_t(0) );
			if( exists )
				++found;
		}
		uint_fast8_t* pColumnIndexes3 = columnIndexes3.data();
		do
		{
			file.read( rgBuffer.data(), chunkSize );
			const size_t charactersRead = file.gcount();
			for( size_t i=0, start=0, tokenIndex=0, columnIndex=0; i<charactersRead; ++i )
			{
				char ch = rgBuffer[i];
				const bool newLineStart = ch=='\r' || ch=='\n' || (i==charactersRead-1 && file.eof());
				if( ch!=',' && !newLineStart )
					continue;

				const uint_fast8_t getToken = pColumnIndexes3[columnIndex];
				if( getToken )
				{
					std::string token(&rgBuffer[start], i-start);
					if( tokens.size()<=tokenIndex )
						tokens.push_back( token );
					else
						tokens[tokenIndex] = token;
					++tokenIndex;
				}

				++columnIndex;
				if( newLineStart )
				{
					if( lineIndex>=startLine )
					{
						while( tokens.size()>tokenIndex )
							tokens.pop_back();
						function(tokens, lineIndex);
					}
					tokenIndex=columnIndex=0;
					if( ++lineIndex>lineCount )
						break;
					if( ch=='\r' && i<charactersRead-1 && rgBuffer[i+1]=='\n' )
						++i;
				}
				start=i+1;
			}
			if( lineIndex>lineCount )
				break;
		}while( !file.eof() );

		return lineIndex;
	}

	size_t File::ForEachLine3( sv pszFileName, const std::function<void(const std::vector<double>&, size_t lineIndex)>& function, const std::set<size_t>& columnIndexes, const size_t maxLines, const size_t startLine, const size_t chunkSize, size_t maxColumnCount, Stopwatch* /*sw*/ )
	{
		std::ifstream file( string(pszFileName).c_str() ); THROW_IFX( file.fail(), IOException(fs::path(pszFileName), "Could not open file") );

		std::vector<char> rgBuffer( chunkSize );
		vector<double> tokens;
		size_t lineIndex = 0;
		vector<uint_fast8_t> columnIndexes3;
		size_t found=0;

		size_t index=0;
		for( size_t i=0; i<maxColumnCount; ++i )
		{
			bool exists = columnIndexes.find(index++)!=columnIndexes.end();
			columnIndexes3.push_back( exists ? uint_fast8_t(1) : uint_fast8_t(0) );
			if( exists )
				++found;
		}
		uint_fast8_t* pColumnIndexes3 = columnIndexes3.data();
		up<std::thread> pThread(nullptr);
		do
		{
			file.read( rgBuffer.data(), chunkSize );
			const size_t charactersRead = file.gcount();
			if( false )
			{
				Stopwatch swStrDupThrough( "strDupThrough" /*, false*/ );
				std::vector<double> rgTest;
				rgTest.reserve( 1500 );
				size_t lineIndex2=0;
				char* pStart = &rgBuffer[0];
				for( ; lineIndex2<startLine; ++lineIndex2 )
					pStart = strchr( &rgBuffer[0], '\n' );

				while( size_t(pStart-&rgBuffer[0])<charactersRead )
				{
					if( *(pStart+1)=='\n' )//1156=next line
						++lineIndex2;
				}
			}
			for( size_t i=0, start=0, tokenIndex=0, columnIndex=0; i<charactersRead; ++i )
			{
				char ch = rgBuffer[i];
				const bool newLineStart = ch=='\r' || ch=='\n' || (i==charactersRead-1 && file.eof());
				if( ch!=',' && !newLineStart )
					continue;

				const uint_fast8_t getToken = pColumnIndexes3[columnIndex];
				if( getToken && lineIndex>=startLine )
				{
					double value = ch=='\n' ? 0.0 : strtod( &rgBuffer[start], nullptr );
					if( tokens.size()<=tokenIndex )
						tokens.push_back( value );
					else
						tokens[tokenIndex] = value;
					++tokenIndex;
				}

				++columnIndex;
				if( newLineStart )
				{
					if( lineIndex>=startLine )
					{
						while( tokens.size()>tokenIndex )
							tokens.pop_back();
						auto threadStart=[tokens,lineIndex,&function]()
						{
							function(tokens, lineIndex);
						};
						if( pThread )
							pThread->join();
						threadStart();
					}
					tokenIndex=columnIndex=0;
					if( ++lineIndex>=maxLines+startLine )
						break;
					if( ch=='\r' && i<charactersRead-1 && rgBuffer[i+1]=='\n' )
						++i;
				}
				start=i+1;
			}
			if( lineIndex>=maxLines+startLine )
				break;
		}while( !file.eof() );
		if( pThread )
			pThread->join();
		return lineIndex-startLine;
	}

	size_t File::ForEachLine4( sv pszFileName, const std::function<void(const std::vector<double>&, size_t lineIndex)>& function, const std::set<size_t>& columnIndexes, const size_t maxLines/*=std::numeric_limits<size_t>::max()*/, const size_t startLine/*=0*/, const size_t /*chunkSize=1073741824*/, size_t maxColumnCount/*=1500*/, Stopwatch* /*sw=nullptr*/, double emptyValue/*=0.0*/ )
	{
		std::ifstream file2( string(pszFileName).c_str(), std::ios::binary | std::ios::ate);
		const size_t fileSize = file2.tellg();
		file2.close();
		std::ifstream file( string(pszFileName).c_str() ); THROW_IFX( file.fail(), IOException(fs::path(pszFileName), "Could not open file") );
		size_t chunkSize2=fileSize;
		std::vector<char> rgBuffer( chunkSize2 );
		vector<double> tokens;
		size_t lineIndex = 0;
		vector<uint_fast8_t> columnIndexes3;
		size_t found=0;
		size_t index=0;
		for( size_t i=0; i<maxColumnCount; ++i )
		{
			bool exists = columnIndexes.find(index++)!=columnIndexes.end();
			columnIndexes3.push_back( exists ? uint_fast8_t(1) : uint_fast8_t(0) );
			if( exists )
				++found;
		}
		uint_fast8_t* pColumnIndexes3 = columnIndexes3.data();

		size_t tokenIndex=0;
		long columnIndex=0;
		do
		{
			file.read( rgBuffer.data(), size_t(chunkSize2)/**/ );

			const size_t charactersRead = file.gcount();
			char* pStart = rgBuffer.data(); auto pEnd = pStart+charactersRead;
			char* p = pStart;
			if( lineIndex==0 && startLine>0 )
			{
				for( ; lineIndex<startLine; ++lineIndex )
					p = strchr( &rgBuffer[0], '\n' );
				++p;
			}

			for( ; p<pEnd; ++p, ++columnIndex )
			{
				const uint_fast8_t getToken = pColumnIndexes3[columnIndex];
				if( !getToken )
				{
					for( ; *p!=',' && *p!='\n' && *p!='\r' ; ++p );
					const bool newLineStart = *p=='\r' || *p=='\n' || ( p==p+charactersRead-1 && file.eof() );
					if( !newLineStart )
						continue;
				}
				else
				{
					var empty = *p==',' || *p=='\n';
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
							size_t exponent = 0;
							for( ;*p >= '0' && *p <= '9'; ++p )
								exponent = exponent*10 + *p - '0';
							if( !positiveExponent )
								exponent*=-1;
							value = value*double( pow(10,exponent) );
						}
						if( negative )
							value = -value;
					}
					if( tokens.size()<=tokenIndex )
						tokens.push_back( value );
					else
						tokens[tokenIndex] = value;
					++tokenIndex;
					const bool newLineStart = *p=='\r' || *p=='\n' || ( p==p+charactersRead-1 && file.eof() );
					if( !newLineStart )
						continue;
				}
				while( tokens.size()>tokenIndex )
					tokens.pop_back();
				function( tokens, lineIndex );
				columnIndex=-1;
				tokenIndex=0;
				if( ++lineIndex-startLine>=maxLines )
					break;
			}
			if( lineIndex-startLine>=maxLines )
				break;
		}while( !file.eof() );
		return lineIndex-startLine;
	}

	std::pair<std::vector<string>,std::set<size_t>> File::LoadColumnNames( sv csvFileName, std::vector<string>& columnNamesToFetch, bool notColumns )
	{
		std::vector<string> columnNames;
		std::set<size_t> columnIndexes;
		auto columnNameFunction = [&columnNamesToFetch,&columnIndexes,&columnNames,&notColumns](sv line)
		{
			auto tokens = Str::Split( string(line) );
			int iToken=0;
			for( var& token : tokens )
			{
				if( (!notColumns && std::find( columnNamesToFetch.begin(), columnNamesToFetch.end(), token)!=columnNamesToFetch.end())
					|| (notColumns && std::find( columnNamesToFetch.begin(), columnNamesToFetch.end(), token)==columnNamesToFetch.end()) )
				{
					columnNames.push_back( token );
					columnIndexes.insert( iToken );
				}
				++iToken;
			}
		};
		IO::File::ForEachLine( string(csvFileName), columnNameFunction, 1 );
		return make_pair( columnNames, columnIndexes );
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