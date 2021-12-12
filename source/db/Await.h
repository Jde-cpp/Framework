#pragma once
#include "Row.h"
#include "../Cache.h"
#include "../coroutine/Awaitable.h"

namespace Jde::DB
{
	struct IDataSource;
	using namespace Coroutine;
	using RowΛ=function<void( const IRow& )noexcept(false)>;
	Τ using CoRowΛ=function<void( sp<T> pResult, const IRow& r )noexcept(false)>;
	struct ISelect
	{
		ISelect( sp<IDataSource> ds )noexcept:_ds{ds}{}
		β Results()noexcept->sp<void> = 0;/*  */
		β OnRow( const IRow& r )noexcept->void=0;
		β SelectCo( string sql, vector<object>&& params, SRCE )noexcept->up<IAwaitable>;
	protected:
		sp<IDataSource> _ds;
	};
/////////////////////////////////////////////////////////////////////////////////////////////////////
	Τ class TSelect : public ISelect
	{
	protected:
		TSelect( IAwaitable& base_, sp<IDataSource> ds, string sql, CoRowΛ<T> fnctn, vector<object> params )noexcept:ISelect{ds}, _base{base_},_sql{move(sql)}, _fnctn{fnctn}, _params{move(params)}{}
		virtual ~TSelect()=0;
		α Select( HCoroutine h )->Task2;
		α Results()noexcept->sp<void> override{ return _pResult; }
		α OnRow( const IRow& r )noexcept->void override{ _fnctn(_pResult,r); }

		IAwaitable& _base;
		sp<T> _pResult{ ms<T>() };
		string _sql;
		CoRowΛ<T> _fnctn;
		vector<object> _params;
	};
	Τ struct SelectAwait final: IAwaitable, TSelect<T>
	{
		SelectAwait( sp<IDataSource> ds, string sql, CoRowΛ<T> fnctn, vector<object> params, SL sl )noexcept:IAwaitable{sl},TSelect<T>( *this, ds, move(sql), fnctn, move(params) ){}
		α await_suspend( HCoroutine h )noexcept->void override{ IAwaitable::await_suspend( h ); TSelect<T>::Select( move(h) ); }
	};

	Τ TSelect<T>::~TSelect(){};
	ⓣ TSelect<T>::Select( HCoroutine h )->Task2
	{
		try
		{
			auto tr = co_await *SelectCo( move(_sql), move(_params), _base._sl );
			_base.Set( _pResult );
		}
		catch( IException& e )
		{
			_base.Set( e.Clone() );
		}
		h.resume();
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////
	class ICacheAwait : public IAwaitable
	{
		using base=IAwaitable;
	public:
		ICacheAwait( string name, SL sl ):base{sl},_name{move(name)}{ ASSERT(_name.size()); }
		virtual ~ICacheAwait()=0;
		α await_ready()noexcept->bool;
		α await_resume()noexcept->TResult;
	private:
		string _name;
		sp<void> _pValue;
	};
	inline ICacheAwait::~ICacheAwait(){};
	Τ struct SelectCacheAwait final: ICacheAwait, TSelect<T>
	{
		SelectCacheAwait( sp<IDataSource> ds, string sql, string cache, CoRowΛ<T> fnctn, vector<object> params, SL sl ):ICacheAwait{cache,sl},TSelect<T>{ *this, ds, move(sql), fnctn, move(params) }{}
		α await_suspend( HCoroutine h )noexcept->void override{ ICacheAwait::await_suspend( h ); TSelect<T>::Select( move(h) ); }
	};
}