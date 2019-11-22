#include "Application.h"
#include <iostream>
namespace Jde
{
	struct OSApp : IApplication
	{
		static void Startup( int argc, char** argv, string_view appName )noexcept;
	protected:
		bool KillInstance( uint processId )noexcept override;
		void SetConsoleTitle( string_view title )noexcept override{ std::cout << "\033]0;" << title << "\007"; }
		void AddSignals()noexcept override;
		bool AsService()noexcept override;
		void OSPause()noexcept override;
	private:
		static void ExitHandler( int s );
	};
}
