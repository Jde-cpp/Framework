#include <jde/framework/io/file.h>
#include <fstream>

#define let const auto
namespace Jde{
	constexpr ELogTags _tags{ ELogTags::IO };
	Ω fileSize( const fs::path& path )ε->uint{
		try{
			return fs::file_size( fs::canonical(path) );
		}
		catch( fs::filesystem_error& e ){
			throw IOException( move(e) );
		}
	}

	α IO::Load( const fs::path& path, SL sl )ε->string{
		CHECK_PATH( path, sl );
		auto size = fileSize( path );
		Trace{ sl, _tags, "Opening {} - {} bytes ", path.string(), size };
		std::ifstream f( path, std::ios::binary ); THROW_IFX(f.fail(), IOException(path, "Could not open file") );
		string y;
		y.reserve( size );
		y.assign( (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>() );  //vexing parse
		return y;
	}

	α IO::LoadBinary( const fs::path& path, SL sl )ε->vector<char>{//fs::filesystem_error
		CHECK_PATH( path, sl );
		auto size = fileSize( path );
		Trace{ sl, _tags, "Opening {} - {} bytes ", path.string(), size };
		std::ifstream f( path, std::ios::binary ); THROW_IFX( f.fail(), IOException(path, "Could not open file", sl) );

		return vector<char>{ (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>() };  //vexing parse
	}

	α IO::SaveBinary( const fs::path& path, const std::vector<char>& data, SL sl )ε->void{
		std::ofstream f( path, std::ios::binary );
		THROW_IFX( f.fail(), IOException(path, "Could not open file", sl) );
		f.write( data.data(), data.size() );
	}
}