#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>

#include "gtest/gtest.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/Cache.h"
#define var const auto
template<class T> using sp = std::shared_ptr<T>;
template<typename T>
constexpr auto ms = std::make_shared<T>;

namespace Jde
{
 	void Startup( int argc, char **argv )noexcept
	{
		var appName = "Tests.Framework"sv;
		OSApp::Startup( argc, argv, appName );
	}
}

int main( int argc, char **argv )
{
#ifdef _MSC_VER
	_crtBreakAlloc = 163;
	 _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    _CrtSetBreakAlloc( 3879 );
#endif
	::testing::InitGoogleTest( &argc, argv );
#ifdef _MSC_VER
	auto x = new char[]{"aaaaaaaaaaaaaaaaaaaaaaaaaa"};
#endif
	Jde::Startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		::testing::GTEST_FLAG(filter) = "CoroutineTests.CoLock";//QLTests.DefTestsFetch
	   result = RUN_ALL_TESTS();
		Jde::IApplication::Instance().Wait();
		Jde::IApplication::CleanUp();
	}
	return result;
}