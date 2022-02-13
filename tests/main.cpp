#include "gtest/gtest.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/Cache.h"
#define var const auto

namespace Jde
{
 	void Startup( int argc, char **argv )noexcept
	{
#ifdef _MSC_VER
		ASSERT( Settings::Get<uint>("workers/drive/threads")>0 )
#endif
		ASSERT( argc>1 && string{argv[1]}=="-c" )
		OSApp::Startup( argc, argv, "Tests.Framework"sv, "Unit Tests description" );
	}
}

int main( int argc, char **argv )
{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		var p=Settings::Get<string>( "testing/tests" );
		var filter = p ? *p : "*";
		::testing::GTEST_FLAG( filter ) = filter;
	   result = RUN_ALL_TESTS();
		IApplication::Shutdown();
		IApplication::Cleanup();
	}
	return result;
}