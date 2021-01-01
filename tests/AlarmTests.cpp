#include "gtest/gtest.h"
#include "../source/coroutine/Alarm.h"
#include "../source/coroutine/Coroutine.h"

#define var const auto
namespace Jde
{
	class AlarmTests : public ::testing::Test
	{
	protected:
		AlarmTests() {}
		~AlarmTests() override{}

		void SetUp() override {}
		void TearDown() override {}
	public:
		//static uint Count;
	};
	uint Count{0};
	using namespace Chrono;
	using std::chrono::milliseconds;

	struct foo
	{
		foo(){ DBG0("foo"sv); }
		~foo(){ DBG0("~foo"sv); }
	};
	Coroutine::TaskVoid Call( Duration d, Coroutine::Handle& handle, bool cancel=false )
	{
		//DBG( "Call {}"sv, ToString(d) );
		//auto pFoo = cancel ? make_unique<foo>() : up<foo>{};

		var start = Clock::now();
		co_await Threading::Alarm::Wait( start+d, handle );
		++Count;
		var actual = duration_cast<milliseconds>(Clock::now()-start).count();
		var expected = duration_cast<milliseconds>( std::max(d, Duration::zero()) ).count();
#ifdef _MSC_VER
	#ifdef NDEBUG
			var threshold = 40;
	#else
			var threshold = 5;
	#endif
#else
		var threshold = 2;
#endif
		DBG( "actual={}, expected={} diff={}"sv, actual, expected, actual-expected );
		ASSERT_DESC( std::abs( actual-expected )<=threshold, format("({}-{}<={}ms", actual, expected, threshold) );
	}
	TEST_F(AlarmTests, Base)
	{
		Coroutine::Handle handle, cancelHandle;
		Call( 5500ms, handle );
		Call( 5ms, handle );
		Call( 1s, handle );
		Call( 3s, cancelHandle, true );
		Threading::Alarm::Cancel( cancelHandle );
		Call( -1s, handle );
		Call( 2s, handle );
		Call( 5500ms, handle );
		DBG0( "Sleeping"sv );
		std::this_thread::sleep_for( 6s );
		ASSERT_EQ( Count, 6 );
	}
}