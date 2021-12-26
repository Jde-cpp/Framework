#pragma once
#include "Row.h"
#include "../Cache.h"
#include "../coroutine/Awaitable.h"

namespace Jde::DB
{
	struct IDataSource;
	using namespace Coroutine;
	using RowΛ=function<void( const IRow& )noexcept(false)>;
	Τ using CoRowΛ=function<void( T& pResult, const IRow& r )noexcept(false)>;
	struct ISelect
	{
		ISelect( sp<IDataSource> ds )noexcept:_ds{ds}{}
		β Results()noexcept->void* = 0;
		β OnRow( const IRow& r )noexcept->void=0;
		β SelectCo( string sql, vector<object>&& params, SRCE )noexcept->up<IAwait>;
	protected:
		sp<IDataSource> _ds;
	};
/////////////////////////////////////////////////////////////////////////////////////////////////////
	Τ class TSelect : public ISelect
	{
	protected:
		TSelect( IAwait& base_, sp<IDataSource> ds, string sql, CoRowΛ<T> fnctn, vector<object> params )noexcept:ISelect{ds}, _base{base_},_sql{move(sql)}, _fnctn{fnctn}, _params{move(params)}{}
		virtual ~TSelect()=0;
		α Select( HCoroutine h )->Task;
		α Results()noexcept->void* override{ return _pResult.release(); }
		α OnRow( const IRow& r )noexcept->void override{ _fnctn(*_pResult,r); }

		IAwait& _base;
		up<T> _pResult{ mu<T>() };
		string _sql;
		CoRowΛ<T> _fnctn;
		vector<object> _params;
	};
	Τ struct SelectAwait final: IAwait, TSelect<T>
	{
		SelectAwait( sp<IDataSource> ds, string sql, CoRowΛ<T> fnctn, vector<object> params, SL sl )noexcept:IAwait{sl},TSelect<T>( *this, ds, move(sql), fnctn, move(params) ){}
		α await_suspend( HCoroutine h )noexcept->void override{ IAwait::await_suspend( h ); TSelect<T>::Select( move(h) ); }
	};

	Τ TSelect<T>::~TSelect(){};
	ⓣ TSelect<T>::Select( HCoroutine h )->Task
	{
		try
		{
			auto tr = co_await *SelectCo( move(_sql), move(_params), _base._sl );
			_base.Set( move(_pResult) );
		}
		catch( IException& e )
		{
			_base.Set( e.Move() );
		}
		h.resume();
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////
	#define Φ Γ auto
	class ICacheAwait : public IAwait
	{
		using base=IAwait;
	public:
		ICacheAwait( string name, SL sl ):base{sl},_name{move(name)}{ ASSERT(_name.size()); }
		virtual ~ICacheAwait()=0;
		Φ await_ready()noexcept->bool;
		Φ await_resume()noexcept->AwaitResult;
	protected:
		sp<void> _pValue;
		string _name;
	};
	inline ICacheAwait::~ICacheAwait(){};
	Τ struct SelectCacheAwait final: ICacheAwait, TSelect<T>
	{
		SelectCacheAwait( sp<IDataSource> ds, string sql, string cache, CoRowΛ<T> fnctn, vector<object> params, SL sl ):ICacheAwait{cache,sl},TSelect<T>{ *this, ds, move(sql), fnctn, move(params) }{}
		α await_suspend( HCoroutine h )noexcept->void override{ ICacheAwait::await_suspend( h ); TSelect<T>::Select( move(h) ); }
		Φ await_resume()noexcept->AwaitResult override;
	};

	ⓣ SelectCacheAwait<T>::await_resume()noexcept->AwaitResult
	{
		auto y = _pValue ? AwaitResult{ _pValue } : IAwait::await_resume();
		if( !_pValue && y.HasValue() )
		{
			sp<T> p{ y. template UP<T>().release() };
			Cache::Set<void>( _name, p );
			y.Set( p );
		}
		return y;
	}
}
#undef Φ