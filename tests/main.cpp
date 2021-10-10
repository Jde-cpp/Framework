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
		OSApp::Startup( argc, argv, appName, "Unit Tests description" );
	}
}

int main( int argc, char **argv )
{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		if( var p=Settings::TryGet<string>("testing/tests"); p )
			::testing::GTEST_FLAG( filter ) = *p;  //"QLTests.*";//QLTests.DefTestsFetch
	   result = RUN_ALL_TESTS();
		//IApplication::Instance().Wait();
		IApplication::CleanUp();
	}
	return result;
}