#include "Database.h"
#include "c_api.h"
#include "GraphQL.h"
#include "DataSource.h"
#include "../application/Application.h"
#include "../io/File.h"
#include "../Settings.h"
#include "../TypeDefs.h"
#include "../Dll.h"
#include "Syntax.h"
#include <boost/container/flat_map.hpp>

#define var const auto
namespace Jde
{
	using boost::container::flat_map;
	using nlohmann::json;

	class DataSourceApi
	{
		DllHelper _dll;
	public:
		DataSourceApi( path path ):
			_dll{ path },
			GetDataSourceFunction{ _dll["GetDataSource"] }
		{}
		decltype(GetDataSource) *GetDataSourceFunction;

		sp<DB::IDataSource> Emplace( string_view connectionString )
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
	sp<DB::Syntax> _pSyntax;
	sp<DB::IDataSource> _pDefault;
	void DB::CleanDataSources()noexcept
	{
		DBG( "CleanDataSources"sv );
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
		DBG( "~CleanDataSources"sv );
	}
	void DB::CreateSchema()noexcept(false)
	{
		auto pDataSource = DB::DataSource();
		var path = Settings::Global().Get<fs::path>( "metaDataPath" );
		INFO( "db meta='{}'"sv, path.string() );
		var j = json::parse( IO::FileUtilities::Load(path) );
		/*var schema =*/ pDataSource->SchemaProc()->CreateSchema( j );
	}
	sp<DB::Syntax> DB::DefaultSyntax()noexcept
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
	void Initialize( path libraryName )noexcept(false)
	{
		static DataSourceApi api{ libraryName };
		_pDefault = shared_ptr<Jde::DB::IDataSource>{ api.GetDataSourceFunction() };
		std::call_once( _singleShutdown, [](){ IApplication::AddShutdownFunction( DB::CleanDataSources ); } );
	}

	sp<DB::IDataSource> DB::DataSource()noexcept(false)
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

	sp<DB::IDataSource> DB::DataSource( path libraryName, string_view connectionString )
	{
		shared_ptr<IDataSource> pDataSource;
		std::unique_lock l{_dataSourcesMutex};
		string key = libraryName.string();
		std::call_once( _singleShutdown, [](){ IApplication::AddShutdownFunction( CleanDataSources ); } );
		auto pApi = _dataSources.emplace( key, sp<DataSourceApi>{new DataSourceApi(libraryName)} ).first;
		return pApi->second->Emplace( connectionString );
	}

#define DS if( auto p = DataSource(); p ) p
#define DS_RET(x) auto p = DataSource(); return !p ? x : p
	void DB::Select( sv sql, std::function<void(const IRow&)> f )noexcept(false)
	{
		DS->Select( sql, f );
	}
	uint DB::ExecuteProc( sv sql, std::vector<DataValue>&& parameters )noexcept(false)
	{
		DS_RET(0)->ExecuteProc( sql, parameters );
	}
	//typedef DB::IDataSource* (*pDataSourceFactory)();
	//typedef shared_ptr<DB::IDataSource> IDataSourcePtr;
}