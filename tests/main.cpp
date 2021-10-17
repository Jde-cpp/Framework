#include "gtest/gtest.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/Cache.h"
#define var const auto

namespace Jde
{
#ifndef MSC_VER
	string OSApp::CompanyName()noexcept{ return "Jde-Cpp"; }
#endif
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
			::testing::GTEST_FLAG( filter ) = *p;
	   result = RUN_ALL_TESTS();
		IApplication::CleanUp();
	}
	return result;
}