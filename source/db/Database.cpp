#include "Database.h"
#include "c_api.h"
#include "GraphQL.h"
#include "DataSource.h"
#include <jde/App.h>
#include <jde/io/File.h>
#include "../Settings.h"
#include <jde/Dll.h>
#include "Syntax.h"
#include <boost/pointer_cast.hpp>

#define var const auto
namespace Jde
{
	using boost::container::flat_map;
	using nlohmann::json;
	using nlohmann::ordered_json;
	static var& _logLevel{ Logging::TagLevel("sql") };

	α DB::LogDisplay( sv sql, const vector<object>* pParameters, string error )noexcept->string
	{
		ostringstream os;
		if( error.size() )
			os << move(error) << std::endl;
		uint prevIndex=0;
		for( uint sqlIndex=0, paramIndex=0, size = pParameters ? pParameters->size() : 0; (sqlIndex=sql.find_first_of('?', prevIndex))!=string::npos && paramIndex<size; ++paramIndex, prevIndex=sqlIndex+1 )
		{
			os << sql.substr( prevIndex, sqlIndex-prevIndex );
			os << DB::ToString( (*pParameters)[paramIndex] );
		}
		if( prevIndex<sql.size() )
			os << sql.substr( prevIndex );
		return os.str();
	}
	α DB::Log( sv sql, const vector<object>* pParameters, SL sl )noexcept->void
	{
		var l = _logLevel.Level;
		Logging::Log( Logging::Message{l, LogDisplay(sql, pParameters, {}), sl} );
	}

	α DB::Log( sv sql, const vector<object>* pParameters, ELogLevel level, string error, SL sl )noexcept->void
	{
		Logging::Log( Logging::Message{level, LogDisplay(sql, pParameters, move(error)), sl} );
	}
	α DB::LogNoServer( string sql, const vector<object>* pParameters, ELogLevel level, string error, SL sl )noexcept->void
	{
		Logging::LogNoServer( Logging::Message{level, LogDisplay(move(sql), pParameters, move(error)), sl} );
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

		α Emplace( str connectionString )->sp<DB::IDataSource>
		{
			std::unique_lock l{ _connectionsMutex };
			auto pDataSource = _pConnections->find( connectionString );
			if( pDataSource == _pConnections->end() )
			{
				auto pNew = sp<Jde::DB::IDataSource>{ GetDataSourceFunction() };
				pNew->SetConnectionString( connectionString );
				pDataSource = _pConnections->emplace( connectionString, pNew ).first;
			}
			return pDataSource->second;
		}
		static sp<flat_map<string,sp<Jde::DB::IDataSource>>> _pConnections; static mutex _connectionsMutex;
	};
	sp<flat_map<string,sp<DB::IDataSource>>> DataSourceApi::_pConnections = make_shared<flat_map<string,sp<DB::IDataSource>>>(); mutex DataSourceApi::_connectionsMutex;
	up<flat_map<string,sp<DataSourceApi>>> _pDataSources = make_unique<flat_map<string,sp<DataSourceApi>>>(); mutex _dataSourcesMutex;
	sp<DB::Syntax> _pSyntax;
	sp<DB::IDataSource> _pDefault;
	up<vector<function<void()>>> _pDBShutdowns = make_unique<vector<function<void()>>>();
	α DB::ShutdownClean( function<void()>& shutdown )noexcept->void
	{
		_pDBShutdowns->push_back( shutdown );
	}
	α DB::CleanDataSources()noexcept->void
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
	#define db DataSource()
	DB::Schema _schema;
	α DB::DefaultSchema()noexcept(false)->Schema&{ return _schema; }
	α DB::CreateSchema()noexcept(false)->void
	{
		var path = Settings::Global().Getɛ<fs::path>( "metaDataPath" );
		INFO( "db meta='{}'"sv, path.string() );
		ordered_json j = json::parse( IO::FileUtilities::Load(path) );
		_schema = db.SchemaProc()->CreateSchema( j, path.parent_path() );
	}
	α DB::DefaultSyntax()noexcept->const DB::Syntax&
	{
		if( !_pSyntax )
		{
			_pSyntax = Settings::Getɛ<string>("db/driver")=="Jde.DB.Odbc.dll"
				? make_shared<Syntax>()
				: make_shared<MySqlSyntax>();
		}
		return *_pSyntax;
	}

	std::once_flag _singleShutdown;
	α Initialize( path libraryName )noexcept(false)->void
	{
		static DataSourceApi api{ libraryName };
		_pDefault = sp<Jde::DB::IDataSource>{ api.GetDataSourceFunction() };
		std::call_once( _singleShutdown, [](){ IApplication::AddShutdownFunction( DB::CleanDataSources ); } );
	}

	α DB::DataSourcePtr()noexcept(false)->sp<IDataSource>
	{
		if( !_pDefault /*&& !IApplication::ShuttingDown()*/ )
		{
			Initialize( Settings::Getɛ<string>("db/driver") );
			string cs{ Settings::Getɛ<string>("db/connectionString") };
			var env = cs.find( '=' )==string::npos ? OSApp::EnvironmentVariable( cs ) : string{};
			_pDefault->SetConnectionString( move(env.empty() ? cs : env) );
		}
		return _pDefault;
	}
	α DB::DataSource()noexcept(false)->IDataSource&
	{
		auto p = DataSourcePtr(); THROW_IF( !p, "No default datasource" );
		return *p;
	}

	α DB::DataSource( path libraryName, string connectionString )->sp<DB::IDataSource>
	{
		sp<IDataSource> pDataSource;
		std::unique_lock l{_dataSourcesMutex};
		string key = libraryName.string();
		std::call_once( _singleShutdown, [](){ IApplication::AddShutdownFunction( CleanDataSources ); } );
		auto pSource = _pDataSources->find( key );
		if( pSource==_pDataSources->end() )
			pSource = _pDataSources->emplace( key, make_shared<DataSourceApi>(libraryName) ).first;
		return pSource->second->Emplace( move(connectionString) );
	}

//#define DS if( auto p = DataSource(); p ) p
	α DB::Select( string sql, function<void(const IRow&)> f, SL sl )noexcept(false)->void{ db.Select(move(sql), f, sl); }
	α DB::Select( string sql, function<void(const IRow&)> f, const vector<object>& values, SL sl )noexcept(false)->void{ db.Select(move(sql), f, values, sl); }

	α DB::IdFromName( sv tableName, string name, SL sl )noexcept->SelectAwait<uint>
	{
		return db.ScalerCo<uint>( format("select id from {} where name=?", tableName), {name}, sl );
	}

	α DB::Execute( string sql, vector<object>&& parameters, SL sl )noexcept(false)->uint
	{
		return db.Execute( move(sql), parameters, sl );
	}

	α DB::SelectName( string sql, uint id, sv cacheName, SL sl )noexcept(false)->CIString
	{
		CIString y;
		if( auto p = cacheName.size() ? Cache::GetValue<uint,CIString>(string{cacheName}, id) : sp<CIString>{}; p )
			y = CIString{ *p };
		if( y.empty() )
			y = Scaler<CIString>( move(sql), {id}, sl ).value_or( CIString{} );
		return y;
	}

	α DB::SelectIds( string sql, const std::set<uint>& ids, function<void(const IRow&)> f, SL sl )noexcept(false)->void
	{
		vector<object> params; params.reserve( ids.size() );
		string s; s.reserve( ids.size()*2 );
		for( var id : ids )
		{
			if( s.size() )
				s+=",";
			s+="?";
			params.emplace_back( id );
		}
		Select( format("{}({})", move(sql), s), f, params, sl );
	}
}