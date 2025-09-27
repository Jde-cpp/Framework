#include <jde/framework/log/MemoryLog.h>
#define let const auto

namespace Jde::Tests{
	struct LogGeneralTests : public ::testing::Test{
	protected:
		LogGeneralTests() {}
		~LogGeneralTests() override{}

		Ω SetUpTestCase()ι->void{ }
		α SetUp()->void override{ Logging::ClearMemory(); }
		α TearDown()->void override {}
	};

	TEST_F( LogGeneralTests, CachedTags ){
		auto& logger = Logging::MemoryLogger();
		let checkTags{ ELogTags::App | ELogTags::Pedantic };
		logger.SetLevel( checkTags, ELogLevel::Trace );
		let unConfiguredTags = ELogTags::Scheduler;
		Trace{ unConfiguredTags, "Need to search" };
		let logMessage = Ƒ( "[{}]tag: {}, minLevel: {}", "MemoryLog", Jde::ToString(unConfiguredTags), "{default}" );
		ASSERT_TRUE( logger.Find(logMessage).size() );
		Logging::ClearMemory();
		Trace{ unConfiguredTags, "no need to search" };
		ASSERT_TRUE( logger.Find(logMessage).empty() );
	}
	TEST_F( LogGeneralTests, ArgsNotCalled ){
		auto& logger = Logging::MemoryLogger();
		let unConfiguredTags = ELogTags::Scheduler;
		logger.SetLevel( unConfiguredTags, ELogLevel::Error );
		auto arg = []()->string{
			return "";
		};
		ASSERT_NO_THROW( Trace(unConfiguredTags, "{}", arg()) );
	}
}
