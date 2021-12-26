#include "Await.h"
#include "DataSource.h"

namespace Jde::DB
{
	α ICacheAwait::await_ready()noexcept->bool
	{
		_pValue = Cache::Get<void>( _name );
		return !!_pValue;
	}

	α ICacheAwait::await_resume()noexcept->AwaitResult
	{
		auto y = _pValue ? AwaitResult{ _pValue } : base::await_resume();
		if( !_pValue && y.HasShared() )
			Cache::Set<void>( _name, y.SP<void>() );
		return y;
	}
}