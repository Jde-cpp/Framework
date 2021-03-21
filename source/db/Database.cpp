#include "Database.h"
#include "c_api.h"
#include "GraphQL.h"
#include "DataSource.h"
#include "../application/Application.h"
#include "../Settings.h"
#include "../TypeDefs.h"
#include "../Dll.h"
#include "Syntax.h"
#include <boost/container/flat_map.hpp>

#define var const auto
namespace Jde::DB
{
	using boost::container::flat_map;

	class DataSourceApi
	{
		DllHelper _dll;
	public:
		DataSourceApi( path path ):
			_dll{ path },
			GetDataSourceFunction{ _dll["GetDataSource"] }
		{}
		decltype(GetDataSource) *GetDataSourceFunction;

		sp<IDataSource> Emplace( string_view connectionString )
		{
			std::unique_lock l{ _connectionsMutex };
			auto pDataSource = _connections.find( string(connectionString) );
			if( pDataSource == _connections.end() )
			{
				auto pNew = shared_ptr<Jde::DB::IDataSource>{ GetDataSourceFunction() };
				pNew->ConnectionString = connectionString;
				pDataSource = _connections.emplace( connectionString, pNew ).first;
			}
			return pDataSource->second;
		}
		static flat_map<string,shared_ptr<Jde::DB::IDataSource>> _connections; static mutex _connectionsMutex;
	};
	flat_map<string,shared_ptr<Jde::DB::IDataSource>> DataSourceApi::_connections; mutex DataSourceApi::_connectionsMutex;
	flat_map<string,sp<DataSourceApi>> _dataSources; mutex _dataSourcesMutex;
	sp<Syntax> _pSyntax;
	sp<IDataSource> _pDefault;
	void CleanDataSources()noexcept
	{
		DBG0( "CleanDataSources"sv );
		ClearQLDataSource();
		_pDefault = nullptr;
		_pSyntax = nullptr;
		{
			unique_lock l2{DataSourceApi::_connectionsMutex};
			DataSourceApi::_connections.clear();
		}
		{
			std::unique_lock l{_dataSourcesMutex};
			_dataSources.clear();
		}
		DBG0( "~CleanDataSources"sv );
	}

	sp<Syntax> DefaultSyntax()noexcept
	{
		if( !_pSyntax )
		{
			_pSyntax = Settings::Global().Get<string>("dbDriver")=="Jde.DB.Odbc.dll"
				? make_shared<Syntax>()
				: make_shared<MySqlSyntax>();
		}
		return _pSyntax;
	}

	std::once_flag _singleShutdown;
	void Initialize( path libraryName )
	{
		static DataSourceApi api{ libraryName };
		_pDefault = shared_ptr<Jde::DB::IDataSource>{ api.GetDataSourceFunction() };
		std::call_once( _singleShutdown, [](){ IApplication::AddShutdownFunction( CleanDataSources ); } );
	}

	sp<IDataSource> DataSource()
	{
		if( !_pDefault )
		{
			Initialize( Settings::Global().Get<string>("dbDriver") );
			var cs = Settings::Global().Get<string>( "connectionString" );//			Initialize( "Jde.DB.Odbc.dll" );
			var env = IApplication::Instance().GetEnvironmentVariable( cs );
			_pDefault->ConnectionString = env.empty() ? cs : env;
		}
		ASSERT( _pDefault );
		return _pDefault;
	}

	sp<IDataSource> DataSource( path libraryName, string_view connectionString )
	{
		shared_ptr<IDataSource> pDataSource;
		std::unique_lock l{_dataSourcesMutex};
		string key = libraryName.string();
		std::call_once( _singleShutdown, [](){ IApplication::AddShutdownFunction( CleanDataSources ); } );
		auto pApi = _dataSources.emplace( key, sp<DataSourceApi>{new DataSourceApi(libraryName)} ).first;
		return pApi->second->Emplace( connectionString );
	}

	typedef IDataSource* (*pDataSourceFactory)();
	typedef shared_ptr<IDataSource> IDataSourcePtr;
}