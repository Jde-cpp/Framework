#pragma once
#include <jde/Log.h>
#include "Row.h"
#include "SchemaProc.h"
#include "../Cache.h"
#include "../coroutine/Awaitable.h"


#define DBLOG(sql,params) Jde::DB::Log( sql, params, sl )
namespace Jde::DB
{
	using RowΛ=function<void( const IRow& )noexcept(false)>;
	template<class T> using CoRowΛ=function<void( sp<T> pCollection, const IRow& r )noexcept(false)>;
	struct ISelect
	{
		β Results()->sp<void> = 0;
		β OnRow( const IRow& r )->void = 0;
	};

	class ICacheAwait : public IAwaitable
	{
		using base=IAwaitable;
	public:
		ICacheAwait( string name, SL sl ):base{sl},_name{move(name)}{ ASSERT(_name.size()); }
		virtual ~ICacheAwait()=0;
		α await_ready()noexcept->bool{ _pValue = Cache::Get<void>( _name ); return !!_pValue; }
		α await_resume()noexcept->TResult{ if( _pValue ) return TaskResult{ _pValue }; auto y = base::await_resume();  if( y.HasValue() ) Cache::Set<void>( _name, y.Get<void>(_sl) ); return y; }
	private:
		string _name;
		sp<void> _pValue;
	};
	inline ICacheAwait::~ICacheAwait(){};

	template<class T>
	class ISelectAwait : public ISelect
	{
	protected:
		ISelectAwait( IAwaitable& base, sp<IDataSource> ds, string sql, CoRowΛ<T> fnctn, vector<object> params )noexcept:_base{base},_ds{ds},_sql{move(sql)}, _fnctn{fnctn}, _params{move(params)}{}
		virtual ~ISelectAwait()=0;
		α Select( HCoroutine h )->Task2;
		α Results()noexcept->sp<void> override{ return _pResult; }
		α OnRow( const IRow& r )->void override{ _fnctn(_pResult,r); }

		IAwaitable& _base;
		sp<IDataSource> _ds;
		sp<T> _pResult{ ms<T>() };
		string _sql;
		CoRowΛ<T> _fnctn;
		vector<object> _params;
	};
	template<class T> ISelectAwait<T>::~ISelectAwait(){};
	template<class T>
	struct SelectCacheAwait final: ICacheAwait, ISelectAwait<T>
	{
		SelectCacheAwait( sp<IDataSource> ds, string sql, string cache, CoRowΛ<T> fnctn, vector<object> params, SL sl ):ICacheAwait{cache,sl},ISelectAwait<T>{ *this, ds, move(sql), fnctn, move(params) }{}
		α await_suspend( HCoroutine h )noexcept->void override{ ICacheAwait::await_suspend( h ); ISelectAwait<T>::Select( move(h) ); }
	};
	template<class T>
	struct SelectAwait final: IAwaitable, ISelectAwait<T>
	{
		SelectAwait( sp<IDataSource> ds, string sql, CoRowΛ<T> fnctn, vector<object> params, SL sl )noexcept:IAwaitable{sl},ISelectAwait<T>( *this, ds, move(sql), fnctn, move(params) ){}
		α await_suspend( HCoroutine h )noexcept->void override{ IAwaitable::await_suspend( h ); ISelectAwait<T>::Select( move(h) ); }
	};

	using namespace Coroutine;
	namespace Types{ struct IRow; }
	struct Γ IDataSource : std::enable_shared_from_this<IDataSource>
	{
		virtual ~IDataSource()=0;
		ⓣ SelectEnum( string tableName, SRCE )noexcept{ return SelectMap<T,string>( format("select id, name from {}", tableName), move(tableName), sl ); }
		ⓣ SelectEnumSync( string tableName, SRCE )noexcept->sp<flat_map<T,string>>{ return Future<flat_map<T,string>>( SelectEnum<T>( move(tableName), sl) ).get(); }
		ẗ SelectMap( string sql, SRCE )noexcept->SelectAwait<flat_map<K,V>>;
		ẗ SelectMap( string sql, string cacheName, SRCE )noexcept->SelectCacheAwait<flat_map<K,V>>;
		ⓣ SelectSet( string sql, vector<object>&& params, SL sl )noexcept->SelectAwait<flat_set<T>>;
		ⓣ SelectSet( string sql, vector<object>&& params, string cacheName, SL sl )noexcept->SelectCacheAwait<flat_set<T>>;

		β SchemaProc()noexcept->sp<ISchemaProc> =0;
		α ScalerNonNull( string sql, vec<object> parameters, SRCE )noexcept(false)->uint;
		//virtual void SetAsynchronous()noexcept(false)=0;

		ⓣ TryScaler( string sql, vec<object> parameters, SRCE )noexcept->optional<T>;
		ⓣ Scaler( string sql, vec<object> parameters, SRCE )noexcept(false)->optional<T>;
		//virtual ⓣ ScalerCo( string&& sql, vec& parameters )noexcept(false){ throw Exception( "Not implemented" ); }

		α TryExecute( string sql, SRCE )noexcept->optional<uint>;
		α TryExecute( string sql, vec<object> parameters, SRCE )noexcept->optional<uint>;
		α TryExecuteProc( string sql, vec<object> parameters, SRCE )noexcept->optional<uint>;

		β Execute( string sql, SRCE )noexcept(false)->uint=0;
		β Execute( string sql, vec<object> parameters, SRCE )noexcept(false)->uint = 0;
		β Execute( string sql, const vector<object>* pParameters, RowΛ* f, bool isStoredProc=false, SRCE )noexcept(false)->uint = 0;
		β ExecuteNoLog( string sql, const vector<object>* pParameters, RowΛ* f=nullptr, bool isStoredProc=false, SRCE )noexcept(false)->uint = 0;
		β ExecuteProc( string sql, vec<object> parameters, SRCE )noexcept(false)->uint=0;
		β ExecuteProc( string sql, vec<object> parameters, RowΛ f, SRCE )noexcept(false)->uint=0;
		β ExecuteProcCo( string sql, vector<object> p, SRCE )noexcept->up<IAwaitable> = 0;
		β ExecuteProcNoLog( string sql, vec<object> parameters, SRCE )noexcept(false)->uint=0;

		α Select( string sql, RowΛ f, vec<object> parameters, SRCE )noexcept(false)->void;
		α Select( string sql, RowΛ f, SRCE )noexcept(false)->void;
		β Select( string sql, RowΛ f, const vector<object>* pValues, SRCE )noexcept(false)->uint=0;
		β SelectNoLog( string sql, RowΛ f, const vector<object>* pValues, SRCE )noexcept(false)->uint=0;
		α TrySelect( string sql, RowΛ f, SRCE )noexcept->bool;
		α Catalog( string sql, SRCE )noexcept(false)->string;

		α CS()const noexcept->str{ return _connectionString; }
		β SetConnectionString( sv x )noexcept->void{ _connectionString = x; }

		//bool Asynchronous{false};

	protected:
		string _connectionString;
	private:
		β SelectCo( ISelect* pAwait, string sql, vector<object>&& params, SRCE )noexcept->up<IAwaitable> = 0;
		template<class T> friend class ISelectAwait;
	};
	inline IDataSource::~IDataSource(){};

	ⓣ IDataSource::TryScaler( string sql, vec<object> parameters, SL sl )noexcept->optional<T>
	{
		optional<T> result;
		Try( [&]{ Select( move(sql), [&result](const IRow& row){ result = row.Get<T>(0); }, parameters, sl);} );
		return result;
	}

	ⓣ IDataSource::Scaler( string sql, vec<object> parameters, SL sl )noexcept(false)->optional<T>
	{
		optional<T> result;
		Select( move(sql), [&result](const IRow& row){ result = row.Get<T>(0); }, parameters, sl );
		return result;
	}

	namespace zInternal
	{
		ẗ ProcessMapRow( sp<void> p, const IRow& row )noexcept(false){ std::static_pointer_cast<flat_map<K,V>>(p)->emplace(row.Get<K>(0), row.Get<V>(1)); }
		ⓣ ProcessSetRow( sp<void> p, const IRow& row )noexcept(false){ std::static_pointer_cast<flat_set<T>>(p)->emplace( row.Get<T>(0) ); }
	}
	// f = []( sp<void> p, const IRow& row )noexcept{ std::static_pointer_cast<T>(p)->emplace(row.Get<K>(0), row.Get<V>(1)); };

	ẗ IDataSource::SelectMap( string sql, SL sl )noexcept->SelectAwait<flat_map<K,V>>
	{
		return SelectAwait<flat_map<K,V>>( shared_from_this(), move(sql), zInternal::ProcessMapRow<K,V>, {}, sl );
	}
	ẗ IDataSource::SelectMap( string sql, string cacheName, SL sl )noexcept->SelectCacheAwait<flat_map<K,V>>
	{
		return SelectCacheAwait<flat_map<K,V>>( shared_from_this(), move(sql), move(cacheName), zInternal::ProcessMapRow<K,V>, {}, sl );
	}
	ⓣ IDataSource::SelectSet( string sql, vector<object>&& params, SL sl )noexcept->SelectAwait<flat_set<T>>
	{
		return SelectAwait<flat_set<T>>( shared_from_this(), move(sql), zInternal::ProcessSetRow<T>, move(params), sl );
	}
	ⓣ IDataSource::SelectSet( string sql, vector<object>&& params, string cacheName, SL sl )noexcept->SelectCacheAwait<flat_set<T>>
	{
		return SelectCacheAwait<flat_set<T>>( shared_from_this(), move(sql), move(cacheName), zInternal::ProcessSetRow<T>, move(params), sl );
	}

	ⓣ ISelectAwait<T>::Select( HCoroutine h )->Task2
	{
		try
		{
			//ms<TShared<T>>(), _fnctn
			auto tr = co_await *_ds->SelectCo( this, move(_sql), move(_params), _base._sl );
			//sp<T> p = tr. template Get<T>();
			_base.Set( _pResult );
		}
		catch( IException& e )
		{
			_base.Set( e.Clone() );
		}
		h.resume();
	}
}