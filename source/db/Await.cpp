#include "Await.h"
#include "DataSource.h"

namespace Jde::DB
{
	α ICacheAwait::await_ready()noexcept->bool
	{
		_pValue = Cache::Get<void>( _name );
		return !!_pValue;
	}

	α ICacheAwait::await_resume()noexcept->TResult
	{
		auto y = _pValue ? TaskResult{ _pValue } : base::await_resume();
		if( !_pValue && y.HasValue() )
			Cache::Set<void>( _name, y.Get<void>(_sl) );
		return y;
	}
}