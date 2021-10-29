#include "Database.h"
#include "c_api.h"
#include "GraphQL.h"
#include "DataSource.h"
#include <jde/App.h>
#include <jde/io/File.h>
#include "../Settings.h"
#include <jde/Dll.h>
#include "Syntax.h"
#include <boost/container/flat_map.hpp>

#define var const auto
namespace Jde
{
	using boost::container::flat_map;
	using nlohmann::json;
	using nlohmann::ordered_json;
	ELogLevel _level = Logging::TagLevel( "sql", [](α l){_level=l;} );

	α DB::Message( sv sql, const std::vector<DataValue>* pParameters, sv error )noexcept->string
	{
		var size = pParameters ? pParameters->size() : 0;
		ostringstream os;
		if( error.size() )
			os << error << "\n";
		uint prevIndex=0;
		for( uint sqlIndex=0, paramIndex=0; (sqlIndex=sql.find_first_of('?', prevIndex))!=string::npos && paramIndex<size; ++paramIndex, prevIndex=sqlIndex+1 )
		{
			os << sql.substr( prevIndex, sqlIndex-prevIndex );
			os << DB::to_string( (*pParameters)[paramIndex] );
		}
		if( prevIndex<sql.size() )
			os << sql.substr( prevIndex );
		return os.str();
	}
	α DB::Log( sv sql, const std::vector<DataValue>* pParameters, const source_location& sl )noexcept->void
	{
		Logging::Log( Logging::Message{_level, Message(sql, pParameters, {}), sl} );
	}

	α DB::Log( sv sql, const std::vector<DataValue>* pParameters, ELogLevel level, sv error, const source_location& sl )noexcept->void
	{
		Logging::Log( Logging::Message{level, Message(sql, pParameters, error), sl} );
	}

	class DataSourceApi
	{
		DllHelper _dll;
	public:
		DataSourceApi( path path ):
			_dll{ path },
			GetDataSourceFunction{ _dll["GetDataSource"] }
		{}
		decltype(GetDataSource) *GetDataSourceFunction;

		sp<DB::IDataSource> Emplace( sv connectionString )
		{
			std::unique_lock l{ _connectionsMutex };
			auto pDataSource = _pConnections->find( string(connectionString) );
			if( pDataSource == _pConnections->end() )
			{
				auto pNew = shared_ptr<Jde::DB::IDataSource>{ GetDataSourceFunction() };
				pNew->SetConnectionString( connectionString );
				pDataSource = _pConnections->emplace( connectionString, pNew ).first;
			}
			return pDataSource->second;
		}
		static sp<flat_map<string,shared_ptr<Jde::DB::IDataSource>>> _pConnections; static mutex _connectionsMutex;
	};
	sp<flat_map<string,sp<DB::IDataSource>>> DataSourceApi::_pConnections = make_shared<flat_map<string,sp<DB::IDataSource>>>(); mutex DataSourceApi::_connectionsMutex;
	up<flat_map<string,sp<DataSourceApi>>> _pDataSources = make_unique<flat_map<string,sp<DataSourceApi>>>(); mutex _dataSourcesMutex;
	sp<DB::Syntax> _pSyntax;
	sp<DB::IDataSource> _pDefault;
	up<vector<function<void()>>> _pDBShutdowns = make_unique<vector<function<void()>>>();
	void DB::ShutdownClean( function<void()>& shutdown )noexcept
	{
		_pDBShutdowns->push_back( shutdown );
	}
	void DB::CleanDataSources()noexcept
	{
		DBG( "CleanDataSources"sv );
		DB::ClearQLDataSource();
		_pDefault = nullptr;
		for( auto p=_pDBShutdowns->begin(); p!=_pDBShutdowns->end(); p=_pDBShutdowns->erase(p) )
			(*p)();
		_pDBShutdowns = nullptr;
		_pSyntax = nullptr;
		{
			unique_lock l2{DataSourceApi::_connectionsMutex};
			DataSourceApi::_pConnections = nullptr;
		}
		{
			std::unique_lock l{_dataSourcesMutex};
			_pDataSources = nullptr;
		}
		DBG( "~CleanDataSources"sv );
	}
	void DB::CreateSchema()noexcept(false)
	{
		auto pDataSource = DB::DataSource();
		var path = Settings::Global().Get<fs::path>( "metaDataPath" );
		INFO( "db meta='{}'"sv, path.string() );
		ordered_json j = json::parse( IO::FileUtilities::Load(path) );
		/*var schema =*/ pDataSource->SchemaProc()->CreateSchema( j, path.parent_path() );
	}
	sp<DB::Syntax> DB::DefaultSyntax()noexcept
	{
		if( !_pSyntax )
		{
			_pSyntax = Settings::Get<string>("db/driver")=="Jde.DB.Odbc.dll"
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
			Initialize( Settings::Get<string>("db/driver") );
			var cs = Settings::Get<string>( "db/connectionString" );
			var env = cs.find( '=' )==string::npos ? IApplication::Instance().GetEnvironmentVariable( cs ) : string{};
			_pDefault->SetConnectionString( env.empty() ? cs : env );
		}
		ASSERT( _pDefault );
		return _pDefault;
	}

	sp<DB::IDataSource> DB::DataSource( path libraryName, sv connectionString )
	{
		shared_ptr<IDataSource> pDataSource;
		std::unique_lock l{_dataSourcesMutex};
		string key = libraryName.string();
		std::call_once( _singleShutdown, [](){ IApplication::AddShutdownFunction( CleanDataSources ); } );
		auto pSource = _pDataSources->find( key );
		if( pSource==_pDataSources->end() )
			pSource = _pDataSources->emplace( key, make_shared<DataSourceApi>(libraryName) ).first;
		return pSource->second->Emplace( connectionString );
	}

#define DS if( auto p = DataSource(); p ) p
#define DS_RET(x) auto p = DataSource(); return !p ? x : p
	void DB::Select( sv sql, std::function<void(const IRow&)> f )noexcept(false){ DS->Select(sql, f); }
	void DB::Select( sv sql, std::function<void(const IRow&)> f, const vector<DataValue>& values )noexcept(false){ DS->Select(sql, f, values); }
	uint DB::ExecuteProc( sv sql, vector<DataValue>&& parameters )noexcept(false)
	{
		DS_RET(0)->ExecuteProc( sql, parameters );
	}
	uint DB::Execute( sv sql, std::vector<DataValue>&& parameters )noexcept(false)
	{
		DS_RET(0)->Execute( sql, parameters );
	}

	CIString DB::SelectName( sv sql, uint id, sv cacheName )noexcept(false)
	{
		CIString y;
		if( auto p = cacheName.size() ? Cache::GetValue<uint,CIString>(string{cacheName}, id) : sp<CIString>{}; p )
			y = CIString{ *p };
		if( y.empty() )
			y = Scaler<CIString>( sql, {id} ).value_or( CIString{} );
		return y;
	}

	α DB::SelectIds( sv sql, const std::set<uint>& ids, std::function<void(const IRow&)> f )noexcept(false)->void
	{
		vector<DataValue> params; params.reserve( ids.size() );
		string str; str.reserve( ids.size()*2 );
		for( var id : ids )
		{
			if( str.size() )
				str+=",";
			str+="?";
			params.emplace_back( id );
		}
		Select( format("{}({})", sql, str), f, params );
	}
}