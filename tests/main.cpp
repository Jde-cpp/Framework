#include "gtest/gtest.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/Cache.h"
#define var const auto
template<class T> using sp = std::shared_ptr<T>;
template<typename T>
constexpr auto ms = std::make_shared<T>;

namespace Jde
{
	//shared_ptr<Settings::Container> SettingsPtr;
 	void Startup( int argc, char **argv )noexcept
	{
		var appName = "Tests.Framework"sv;
		OSApp::Startup( argc, argv, appName );
		//string appName{ sv2 };
/*		std::filesystem::path settingsPath{ fmt::format("{}.json", appName) };
		if( !fs::exists(settingsPath) )
			settingsPath = std::filesystem::path( fmt::format("../{}.json", appName) );
		Settings::SetGlobal( std::make_shared<Jde::Settings::Container>(settingsPath) );
		InitializeLogger( appName );
		Cache::CreateInstance();*/
	}
}
int main( int argc, char **argv )
{
	::testing::InitGoogleTest( &argc, argv );
	Jde::Startup( argc, argv );
	auto result = EXIT_FAILURE;
	//if( p )
	{
		::testing::GTEST_FLAG(filter) = "AlarmTests.*";
	   result = RUN_ALL_TESTS();
		Jde::IApplication::Instance().Wait();
		Jde::IApplication::CleanUp();
		//p = nullptr;
	}
	return result;
}