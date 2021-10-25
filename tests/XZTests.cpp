#include "gtest/gtest.h"
#include <jde/App.h>
#include <jde/markets/Exports.h>
#include "../source/coroutine/Awaitable.h"
#include "../../XZ/source/XZ.h"

#include "../../MarketLibrary/source/types/proto/bar.pb.h"

#define var const auto

namespace Jde::DB
{
	struct XZTests : ::testing::Test
	{
	protected:
		XZTests()noexcept {}
		//~XZTests()noexcept override{}

		//α SetUp()noexcept->void override{}
		//α TearDown()noexcept->void override{}
	};

	std::condition_variable_any cv;
	std::shared_mutex mtx;
	auto Read()
	{
		std::shared_lock l{ mtx };
		cv.notify_one();
	}

	TEST_F( XZTests, IntrospectionChildTests )
	{
		Threading::SetThreadDscrptn( "Test" );
		std::thread t{ []{ Threading::SetThreadDscrptn("Main"); IApplication::Pause(); } };
		std::this_thread::yield();
		var path = fs::path{ "C:\\ProgramData\\Jde-cpp\\TwsWebSocket\\securities\\nyse\\XOM" }/"2021-09.dat.xz";
		Future<Markets::Proto::BarFile>( IO::Zip::XZ::ReadProto<Markets::Proto::BarFile>(path) ).get();
		//std::this_thread::sleep_for( 60s );
		DBG( "done" );
	}
} 