#include "Cache.h"

namespace Jde
{
	sp<Cache> _pInstance;
	sp<Cache> Cache::GetInstance()noexcept { return _pInstance; }
	void Cache::CreateInstance()noexcept
	{
		ASSERT( !_pInstance );
		_pInstance = sp<Cache>( new Cache() );
		IApplication::AddShutdown( _pInstance );
	}
	void Cache::Shutdown()noexcept
	{
		DBG0( "Cache::Shutdown"sv );
		_pInstance = nullptr;
	}
}