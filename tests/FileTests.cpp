#include <jde/framework/io/FileAwait.h>
#include "../source/coroutine/Lock.h"
#include <boost/uuid.hpp>
#include <boost/endian/conversion.hpp>
#include <jde/framework/thread/execution.h>

#define let const auto

using boost::uuids::uuid;

namespace Jde::IO::Tests{
	Ω File()->fs::path{ return Settings::FindPath("/testing/file").value_or(fs::temp_directory_path()/"test.txt"); }

	struct FileTests : public ::testing::Test{
	protected:
		FileTests() {}
		~FileTests() override{}

		α SetUp()->void override{
			Information{ELogTags::Test, "{}", File().string()};
			fs::create_directories( File().parent_path() );
		}
		α TearDown()->void override {}
	};

	Ω write( uuid guid1, uuid guid2, Vector<uuid>& written, SRCE )->LockKeyAwait::Task{
		auto l = co_await LockKeyAwait{ File().string() };
		[sl]( uuid guid1, uuid guid2, Vector<uuid>& written, [[maybe_unused]]CoLockGuard l )->VoidAwait::Task {
			try{
				co_await IO::WriteAwait{ File(), Ƒ("{}\n{}\n", to_string(guid1), to_string(guid2)), guid1==uuid{}, sl };
			}
			catch( IException& e ){
				e.Log();
				e.Throw();
			}
			written.push_back( guid1 );
			written.push_back( guid2 );
		}( guid1, guid2, written, move(l) );
	}
	Ω read( Vector<uuid>& readValues, SRCE )->TAwait<string>::Task{
		let content = co_await IO::ReadAwait{ File(), false, sl };
		let guidStrings = Str::Split( content, '\n' );
		ul l{ readValues.Mutex };
		for( auto&& guid : guidStrings )
			readValues.push_back( boost::uuids::string_generator{}(string{guid}), l );
	};

	TEST_F(FileTests, WriteRead){
		ASSERT_TRUE( IO::ChunkByteSize()<74 ); //guid+\n*2
		ASSERT_TRUE( IO::ThreadSize()>1 ); //guid+\n

		array<uuid,1024> guids;
		for( uint i=0; i<guids.size(); ++i ){
			auto suffix = boost::endian::endian_reverse( i );
			((uint*)guids[i].data())[1] = suffix;
		}

		Vector<uuid> written;
		Post( [&written, &guids](){
			for( uint i=0; i<guids.size(); i+=2 ){
				write( guids[i], guids[i+1], written );
			}
		});
//		std::this_thread::sleep_for( 1s );
		while( written.size()<guids.size() )
			std::this_thread::sleep_for( 10ms );

		Vector<uuid> readValues;
		read( readValues );
		while( readValues.empty() )
			std::this_thread::sleep_for( 10ms );

		ASSERT_TRUE( readValues.size() == guids.size() );
		readValues.visit( [&](const uuid& guid){
			ASSERT_TRUE( find(guids, guid)!=guids.end() );
		});
	}
}