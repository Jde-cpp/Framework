#include "DiskMirror.h"
#include "../Defines.h"

namespace Jde::IO
{
	using namespace std::chrono_literals;

	DiskMirror::DiskMirror( path /*path*/ ):
		Interrupt("Mirror", Duration{} )
	{
		//_watcher( path, this )
	}

	void DiskMirror::OnChange( path path )noexcept
	{
		var tarPath = GetRootTar( path );
		if( tarPath!=path )
		{
			_changes.emplace( tarPath, std::chrono::steady_clock::now() );
		}
	}

	void DiskMirror::OnTimeout()noexcept
	{
		var updateTime = std::chrono::steady_clock::now()-_waitTime;
		//TODO - get waitTime from config
		//var done = TODO
	}
	void DiskMirror::OnAwake()noexcept
	{
		CRITICAL( "OnAwake for what reason?" );
	}


	fs::path DiskMirror::GetRootTar( path path )noexcept
	{
		return path;//todo fix this, return path if path==tarPath
	}
}