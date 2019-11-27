#include "stdafx.h"
#include "Database.h"
#include "../application/Application.h"
#include "../Settings.h"
#include "../Dll.h"
#include "c_api.h"

namespace Jde::DB
{
	class DataSourceApi
	{
		DllHelper _dll;
	public:
		DataSourceApi( const fs::path& path ):
			_dll{ path },
			GetDataSourceFunction{ _dll["GetDataSource"] }
		{}
		decltype(GetDataSource) *GetDataSourceFunction;

		sp<IDataSource> Emplace( string_view connectionString )
		{
			std::unique_lock l{_connectionsMutex};
			auto pDataSource =_connections.find( string(connectionString) );
			if( pDataSource == _connections.end() )
			{
				auto pNew = shared_ptr<Jde::DB::IDataSource>{ GetDataSourceFunction() };
				pNew->ConnectionString = connectionString;
				pDataSource =_connections.emplace( connectionString, pNew ).first;
			}
			return pDataSource->second;
		}
		static map<string,shared_ptr<Jde::DB::IDataSource>> _connections; static mutex _connectionsMutex;
	};
	std::atomic<bool> _addedClean = false;//once_flag?
	map<string,shared_ptr<Jde::DB::IDataSource>> DataSourceApi::_connections; mutex DataSourceApi::_connectionsMutex;
	map<string,sp<DataSourceApi>> _dataSources; mutex _dataSourcesMutex;
	shared_ptr<Jde::DB::IDataSource> _dataSource;
	void CleanDataSources()noexcept
	{
		DBG0( "CleanDataSources" );
		_dataSource = nullptr;
		std::unique_lock l{_dataSourcesMutex};
		_dataSources.clear();
	}
	void Initialize( const fs::path& libraryName )
	{
		static DataSourceApi api{ libraryName };
		_dataSource = shared_ptr<Jde::DB::IDataSource>{ api.GetDataSourceFunction() };
		if( !_addedClean )
		{
			_addedClean = true;
			IApplication::AddShutdownFunction( CleanDataSources );
		}
	}

	shared_ptr<IDataSource> DataSource()
	{
		if( !_dataSource )
		{
			Initialize( Settings::Global().Get<string>("dbDriver") );
			_dataSource->ConnectionString = Settings::Global().Get<string>( "connectionString" );//			Initialize( "Jde.DB.Odbc.dll" );
		}
		ASSERT( _dataSource );
		return _dataSource;
	}

	shared_ptr<IDataSource> DataSource( const fs::path& libraryName, string_view connectionString )
	{
		shared_ptr<IDataSource> pDataSource;
		std::unique_lock l{_dataSourcesMutex};
		string key = libraryName.string();
		if( !_addedClean )
		{
			_addedClean = true;
			IApplication::AddShutdownFunction( CleanDataSources );
		}
		auto pApi = _dataSources.emplace( key, sp<DataSourceApi>{new DataSourceApi(libraryName)} ).first;
		return pApi->second->Emplace( connectionString );
	}

	typedef IDataSource* (*pDataSourceFactory)();
	typedef shared_ptr<IDataSource> IDataSourcePtr; 
}