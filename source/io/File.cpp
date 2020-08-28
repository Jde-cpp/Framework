#include "File.h"

#include <cmath>
#include <fstream>
#include <streambuf>
#include <thread>
#include <unordered_set>
#include <spdlog/fmt/ostr.h>
#include "../TypeDefs.h"
#include "../Stopwatch.h"
#include "../StringUtilities.h"
#define var const auto

namespace Jde::IO
{
	namespace FileUtilities
	{
		void ForEachItem( const fs::path& path, std::function<void(const fs::directory_entry&)> function )
		{
			for( const auto& dirEntry : fs::directory_iterator(path) )
     			function( dirEntry );
		}

		std::unique_ptr<std::set<fs::directory_entry>> GetDirectory( const fs::path& directory )
		{
			auto items = make_unique<std::set<fs::directory_entry>>();
			Jde::IO::FileUtilities::ForEachItem( directory, [&items]( fs::directory_entry item ){items->emplace(item);} );
			return items;
		}

		// set<fs::directory_entry> GetDirectoryRecursive( const fs::path& directory, set<fs::directory_entry>& values )noexcept
		// {
		// 	for( const auto& entry : fs::directory_iterator(path) )
		// 	{
		// 		values.emplace( item );
		// 		if( fs::is_directory(item) )
		// 			GetDirectoryRecursive( item.path(), value );
		// 	};
		// 	Jde::IO::FileUtilities::ForEachItem( directory, func );
		// 	return pItems;
		// }
		std::unique_ptr<std::set<fs::directory_entry>> GetDirectories( const fs::path& directory, std::unique_ptr<std::set<fs::directory_entry>> pItems )
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

		size_t GetFileSize( const fs::path& path )noexcept(false)//fs::filesystem_error
		{
			return fs::file_size( fs::canonical(path) );
		}

		unique_ptr<vector<char>> LoadBinary( const fs::path& path )noexcept(false)//fs::filesystem_error
		{
			IOException::TestExists( path );
			auto size = GetFileSize( path );
			TRACE( "Opening {} - {} bytes "sv, path.string(), size );
			ifstream f( path, std::ios::binary );
			if( f.fail() )
				THROW( IOException("Could not open file '{}'", path.string()) );

			//auto pResult = make_unique<vector<char>>(); pResult->reserve( size );
			return make_unique<vector<char>>( (istreambuf_iterator<char>(f)), istreambuf_iterator<char>() );  //vexing parse
			//return move(pResult);
		}
		void SaveBinary( const fs::path& path, const std::vector<char>& data )noexcept(false)
		{
			ofstream f( path, std::ios::binary );
			if( f.fail() )
				THROW( Exception(format("Could not open file '{}'", path.string())) );

			f.write( data.data(), data.size() );
		}
		void Save( const fs::path& path, const std::string& value, std::ios_base::openmode openMode )noexcept(false)
		{
			ofstream f( path, openMode );
			if( f.fail() )
				THROW( IOException(path, "Could not open file") );

			f.write( value.c_str(), value.size() );
		}
		void Compression::Save( const fs::path& path, const vector<char>& data )
		{
			SaveBinary( path, data );
			Compress( path );
		}
		unique_ptr<vector<char>> Compression::LoadBinary( const fs::path& uncompressed, const fs::path& compressed, bool setPermissions, bool leaveUncompressed )noexcept(false)
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
			GetDefaultLogger()->trace( "removed {}.", (leaveUncompressed ? compressedPath.string() : uncompressedFile.string()) );
			return pResult;
		}
		fs::path Compression::Compress( const fs::path& path, bool deleteAfter )noexcept(false)
		{
			var command = CompressCommand( path );
			GetDefaultLogger()->trace( command );
			if( system(command.c_str())==-1 )
				THROW( Exception(format("{} failed.", command)) );
			if( deleteAfter && !CompressAutoDeletes() )
			{
				GetDefaultLogger()->trace( "removed {}.", path.string() );
				VERIFY( fs::remove(path) );
			}
			auto compressedFile = path;
			return compressedFile.replace_extension( format("{}{}", path.extension().string(),Extension()) );
		}
// find . -depth -name '*.zip' -execdir /usr/bin/unzip -n {} \; -delete
// sudo find . -type d -exec chmod 777 {} \;
// sudo find . -type f -exec chmod 666 {} \;

		void Compression::Extract( const fs::path& path )noexcept(false)
		{
			auto destination = string( path.parent_path().string() );
			fs::path compressedFile = path;
			if( compressedFile.extension()!=Extension() )
				compressedFile.replace_extension( format("{}{}", compressedFile.extension().string(),Extension()) );

			auto command = ExtractCommand( compressedFile, destination );// -y -bsp0 -bso0
			GetDefaultLogger()->trace( command.c_str() );
			if( system(command.c_str())==-1 )
				THROW( IOException(format("{} failed.", command)) );
		}
	}

	/*
	std::wstring File::ToString( std::wstring file )
	{
		wifstream t( boo st::locale::conv::utf_to_utf<char,wchar_t>(file) );
		t.seekg( 0, ios::end );
		wstring str;
		str.reserve( t.tellg() );
		t.seekg( 0, ios::beg );

		str.assign( (istreambuf_iterator<wchar_t>(t)), istreambuf_iterator<wchar_t>() );
		return str;
	}*/
	std::string FileUtilities::ToString( const fs::path& filePath )
	{
		auto fileSize = GetFileSize( filePath );
		ifstream t( filePath );
		string str;
		str.reserve( fileSize );
		str.assign( (istreambuf_iterator<char>(t)), istreambuf_iterator<char>() );
		return str;
	}

	vector<string> FileUtilities::LoadColumnNames( const fs::path& csvFileName )
	{
		std::vector<string> columnNames;
		auto columnNameFunction = [&columnNames](string line){ columnNames = StringUtilities::Split<char>(line); };
		IO::File::ForEachLine<char>( csvFileName.string(), columnNameFunction, 1 );
		return columnNames;
	}

	void File::ForEachLine( string_view filePath, const std::function<void(string_view)>& function, const size_t lineCount )
	{
		std::ifstream file( string(filePath).c_str() );
		if( file.fail() )
			THROW( Exception(format("Could not open file '{}'", filePath).c_str()) );
		//String line;
		std::string line;

		for( size_t index = 0; index<lineCount && getline<char>(file, line); ++index )
			function( line );
	}
	size_t File::ForEachLine( string_view pszFileName, const std::function<void(const std::vector<string>&, size_t lineIndex)>& function, const std::set<size_t>& columnIndexes, const size_t maxLines/*=std::numeric_limits<size_t>::max()*/, const size_t startLine/*=0*/, const size_t /*chunkSize=1073741824*/, size_t maxColumnCount/*=1500*/ )
	{
		ifstream file2( string(pszFileName).c_str(), ios::binary | ios::ate );
		const size_t fileSize = file2.tellg();
		file2.close();
		std::ifstream file( string(pszFileName).c_str() );
		if( file.fail() )
			THROW( Exception(format("Could not open file '{}'", pszFileName).c_str()) );
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

	size_t File::ForEachLine2( const fs::path& path, const std::function<void(const std::vector<std::string>&, size_t lineIndex)>& function, const std::set<size_t>& columnIndexes, const size_t lineCount/*=std::numeric_limits<size_t>::max()*/, const size_t startLine, const size_t chunkSize/*=2^30*/, Stopwatch* /* pStopwatch*/ )noexcept(false)
	{
		//FILE* pFile = fopen( file.c_str(), "r" );
		std::ifstream file( path.string() );
		if( file.fail() )
			THROW( IOException("Could not open file '{}'", path.string()) );


		std::vector<char> rgBuffer( chunkSize );
		vector<string> tokens;

		size_t lineIndex = 0;
		//unique_ptr<Stopwatch> swForLoop( new Stopwatch(pStopwatch, StopwatchTypes::ReadFile, String("forLoop")) );
		//unordered_set<size_t> columnIndexes2;
		//for( auto i : columnIndexes )
		//	columnIndexes2.insert(i);
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
		//const size_t columnIndexes3Size = columnIndexes3.size();
		uint_fast8_t* pColumnIndexes3 = columnIndexes3.data();
		do
		{
			//swForLoop->Finish();
			//Stopwatch swRead( pStopwatch, StopwatchTypes::ReadFile, String("ifstream") );
			file.read( rgBuffer.data(), chunkSize );
			//swRead.Finish();
			//swForLoop = unique_ptr<Stopwatch>( new Stopwatch( pStopwatch, StopwatchTypes::ReadFile, String("forLoop") ) );
			const size_t charactersRead = file.gcount();
			//auto pLastIndex = columnIndexes.begin();
			for( size_t i=0, start=0, tokenIndex=0, columnIndex=0; i<charactersRead; ++i )
			{
				//swForLoop->Finish();
				//Stopwatch swReadBuffer( pStopwatch, StopwatchTypes::ReadFile, String("rgBuffer[i]") );
				char ch = rgBuffer[i];
				const bool newLineStart = ch=='\r' || ch=='\n' || (i==charactersRead-1 && file.eof());
				if( ch!=',' && !newLineStart )
				{
					//swReadBuffer.Finish();
					//swForLoop = unique_ptr<Stopwatch>( new Stopwatch( pStopwatch, StopwatchTypes::ReadFile, String("forLoop") ) );
					continue;
				}
				//swReadBuffer.Finish();
				//swForLoop->Finish();
				//Stopwatch find( pStopwatch, StopwatchTypes::ReadFile, String("columnIndexes.find") );
				const uint_fast8_t getToken = pColumnIndexes3[columnIndex];
				//find.Finish();
				//Stopwatch find2( pStopwatch, StopwatchTypes::ReadFile, String("columnIndexes.find2") );
				//find2.Finish();
				if( getToken )
				{
					//Stopwatch createToken( pStopwatch, StopwatchTypes::ReadFile, String("createToken") );
					std::string token(&rgBuffer[start], i-start);
					if( tokens.size()<=tokenIndex )
						tokens.push_back( token );
					else
						tokens[tokenIndex] = token;
					++tokenIndex;
				}

				//swForLoop = unique_ptr<Stopwatch>( new Stopwatch( pStopwatch, StopwatchTypes::ReadFile, String("forLoop") ) );
				++columnIndex;
				if( newLineStart )
				{
					if( lineIndex>=startLine )
					{
						while( tokens.size()>tokenIndex )
						{
							tokens.pop_back();
							//tokens.shrink_to_fit();
						}
						//swForLoop->Finish();
						function(tokens, lineIndex);
						//swForLoop = unique_ptr<Stopwatch>( new Stopwatch( pStopwatch, StopwatchTypes::ReadFile, String("forLoop") ) );
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

	size_t File::ForEachLine3( string_view pszFileName, const std::function<void(const std::vector<double>&, size_t lineIndex)>& function, const std::set<size_t>& columnIndexes, const size_t maxLines, const size_t startLine, const size_t chunkSize, size_t maxColumnCount, Stopwatch* /*sw*/ )
	{
		std::ifstream file( string(pszFileName).c_str() );
		if( file.fail() )
			THROW( Exception(format("Could not open file '{}'", pszFileName).c_str()) );


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
		unique_ptr<std::thread> pThread(nullptr);
		//bool first = true;
		do
		{
			file.read( rgBuffer.data(), chunkSize );
			const size_t charactersRead = file.gcount();
			//auto pLastIndex = columnIndexes.begin();
				//Stopwatch swSub( sw, StopwatchTypes::Calculate, L"getToken", false );  Stopwatch swSW( sw, StopwatchTypes::Calculate, L"Stopwatch", false ); swSW.Finish();// 7.6/15.35
			//24 seconds getting every float allocating rgTest.
			if( false )
			{
				Stopwatch swStrDupThrough( "strDupThrough" /*, false*/ );
				std::vector<double> rgTest;
				rgTest.reserve( 1500 );
				size_t lineIndex2=0;
				char* pStart = &rgBuffer[0];
				for( ; lineIndex2<startLine; ++lineIndex2 )
					pStart = strchr( &rgBuffer[0], '\n' );
				//int iVector=0;

				while( size_t(pStart-&rgBuffer[0])<charactersRead )
				{
					//if(  )
					//	rgTest.push_back( strtod( pStart+1, &pStart ) );
					if( *(pStart+1)=='\n' )//1156=next line
					{
						//rgTest.push_back( 0.0 );
						//rgTest.clear();
						++lineIndex2;
					}
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
					//Stopwatch swSub( sw, StopwatchTypes::Calculate, L"getToken", false );  Stopwatch swSW( sw, StopwatchTypes::Calculate, L"Stopwatch", false ); swSW.Finish();// 7.6/15.35
					//Stopwatch swSW( sw, StopwatchTypes::Calculate, L"Stopwatch", false ); swSW.Finish();
					//Stopwatch swConvert( sw, StopwatchTypes::Calculate, L"strtod", false );  	// 9.0/4:00:00
					//if( tokenIndex==1154 )
						//pLogger->debug( "new line" );
					double value = ch=='\n' ? 0.0 : strtod( &rgBuffer[start], nullptr );
					//swConvert.Finish();
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
						//pThread = unique_ptr<std::thread>(new thread(threadStart) );
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

	size_t File::ForEachLine4( string_view pszFileName, const std::function<void(const std::vector<double>&, size_t lineIndex)>& function, const std::set<size_t>& columnIndexes, const size_t maxLines/*=std::numeric_limits<size_t>::max()*/, const size_t startLine/*=0*/, const size_t /*chunkSize=1073741824*/, size_t maxColumnCount/*=1500*/, Stopwatch* /*sw=nullptr*/, double emptyValue/*=0.0*/ )
	{
		ifstream file2( string(pszFileName).c_str(), ios::binary | ios::ate);
		const size_t fileSize = file2.tellg();
		file2.close();
		std::ifstream file( string(pszFileName).c_str() );
		if( file.fail() )
			THROW( Exception(format("Could not open file '{}'", pszFileName).c_str()) );
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
		//bool newLineStart = false;
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

	std::pair<std::vector<string>,std::set<size_t>> File::LoadColumnNames( string_view csvFileName, std::vector<string>& columnNamesToFetch, bool notColumns )
	{
		std::vector<string> columnNames;
		std::set<size_t> columnIndexes;
		auto columnNameFunction = [&columnNamesToFetch,&columnIndexes,&columnNames,&notColumns](string_view line)
		{
			auto tokens = StringUtilities::Split( string(line) );
			int iToken=0;
			for( const auto& token : tokens )
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

	size_t File::GetFileSize( std::ifstream& file )
	{
		file.clear();   //  Since ignore will have set eof.
		file.seekg( 0, std::ios_base::beg );
		file.ignore( std::numeric_limits<std::streamsize>::max() );
		auto fileSize = file.gcount();
		file.clear();   //  Since ignore will have set eof.
		file.seekg( 0, std::ios_base::beg );
		return fileSize;
	}

	std::string FileUtilities::DateFileName( uint16 year, uint8 month, uint8 day )noexcept
	{
		auto path = std::to_string(year);
		if( month>0 )
		{
			path = format( "{}-{:=02d}", path, month );
			if( day>0 )
				path = format( "{}-{:=02d}", path, day );
		}
		return path;
	}

	tuple<uint16,uint8,uint8> FileUtilities::ExtractDate( const fs::path& path )noexcept
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
}