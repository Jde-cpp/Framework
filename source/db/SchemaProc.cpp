#include "SchemaProc.h"
#include <nlohmann/json.hpp>
#include <boost/container/flat_map.hpp>
#include <jde/Str.h>
#include <jde/io/File.h>
#include "Database.h"
#include "DataSource.h"
#include "Syntax.h"
#include "types/Table.h"

#define var const auto
namespace Jde::DB
{
	using boost::container::flat_map;
	using nlohmann::json;
	using nlohmann::ordered_json;
	struct IDataSource;
	string UniqueIndexName( const Index& index, ISchemaProc& dbSchema, bool uniqueName, const vector<Index>& indexes )noexcept(false);

	string AbbrevName( sv schemaName )
	{
		auto fnctn = []( var& word )
		{
			ostringstream os;
			for( var ch : word )
			{
				if( (ch!='a' && ch!='e' && ch!='i' && ch!='o' && ch!='u') || os.tellp() == std::streampos(0) )
					os << ch;
			}
			return word.size()>2 && os.str().size()<word.size()-1 ? os.str() : word;
		};
		var splits = Str::Split( DB::Schema::ToSingular(schemaName), '_' );
		ostringstream name;
		for( uint i=1; i<splits.size(); ++i )
		{
			if( i>1 )
				name << '_';
			name << fnctn( splits[i] );
		}
		return name.str();
	}
#define _syntax DB::DefaultSyntax()
	Schema ISchemaProc::CreateSchema( const ordered_json& j, path relativePath )noexcept(false)
	{
		var dbTables = LoadTables();

		auto dbIndexes = LoadIndexes();
		var fks = LoadForeignKeys();
		var procedures = LoadProcs();
		flat_map<string, Table> parentTables;
		Schema schema;
		if( j.contains("$types") )
		{
			for( var& [columnName, column] : j.find("$types")->items() )
				schema.Types.emplace( columnName, Column{columnName, column, schema.Types, parentTables, j} );
		}
		function<void(sv,const json&)> addTable;
		addTable = [&]( sv key, const json& item )
		{
			var parentId = item.contains("parent") ? item.find("parent")->get<string>() : string{};
			if( parentId.size() && parentTables.find(parentId)==parentTables.end() )
			{
				THROW_IF( j.find(parentId)==j.end(), "Could not find '{}' parent {}", key, parentId );
				addTable( parentId, *j.find(parentId) );
			}
			parentTables.emplace( key, Table{key, item, parentTables, schema.Types, j} );
		};
		for( var& [key, value] : j.items() )
		{
			if( var name = Schema::FromJson(key); !key.starts_with("$") )
				schema.Tables.emplace( name, make_shared<Table>(name, value, parentTables, schema.Types, j) );
			else if( key!="$types" && key!="$scripts" && parentTables.find(key)==parentTables.end() )
				addTable( key, value );
		}
		for( var& [tableName, pTable] : schema.Tables )
		{
			if( var pNameDBTable=dbTables.find(tableName); pNameDBTable!=dbTables.end() )
			{
				for( auto& column : pTable->Columns )
				{
					var& dbTable = pNameDBTable->second;
					var pDBColumn = dbTable.FindColumn( column.Name ); if( !pDBColumn ){ ERR("Could not find db column {}", column.Name); continue; }
					if( pDBColumn->Default.empty() && column.Default.size() && column.Default!="$now" )
						_pDataSource->TryExecute( _syntax.AddDefault(tableName, column.Name, column.Default) );
				}
			}
			else
			{
				_pDataSource->Execute( pTable->Create(_syntax) );
				DBG( "Created table '{}'."sv, tableName );
				if( pTable->HaveSequence() )
				{
					for( auto pTableIndex : LoadIndexes({}, pTable->Name) )
						dbIndexes.push_back( pTableIndex );
				}
			}

			for( var& index : pTable->Indexes )
			{
				if( find_if(dbIndexes.begin(), dbIndexes.end(), [&](var& db){ return db.TableName==index.TableName && db.Columns==index.Columns;} )!=dbIndexes.end() )
					continue;
				var name = UniqueIndexName( index, *this, _syntax.UniqueIndexNames(), dbIndexes );
				var indexCreate = index.Create( name, tableName, _syntax );
				_pDataSource->Execute( indexCreate );
				dbIndexes.push_back( Index{name, tableName, index} );
				DBG( "Created index '{}.{}'."sv, tableName, name );
			}
			if( var procName = pTable->InsertProcName(); procName.size() && procedures.find(procName)==procedures.end() )
			{
				var procCreate = pTable->InsertProcText( _syntax );
				_pDataSource->Execute( procCreate );
				DBG( "Created proc '{}'."sv, pTable->InsertProcName() );
			}
		}
		if( j.contains("$scripts") )
		{
			for( var& nameObj : *j.find("$scripts") )
			{
				auto name = fs::path{ nameObj.get<string>() };
				var procName = name.stem();
				if( procedures.find(procName.string())!=procedures.end() )
					continue;
				if( _syntax.ProcFileSuffix().size() )
					name = name.parent_path()/( name.stem().string()+string{_syntax.ProcFileSuffix()}+name.extension().string() );
				var path = relativePath/name; THROW_IF( !fs::exists(path), "Could not find {}"sv, path.string() );
				var text = IO::FileUtilities::Load( path );
				DBG( "Executing '{}'"sv, path.string() );
				var queries = Str::Split( text, CIString{"\ngo"sv} );
				for( var& query : queries )
				{
					ostringstream os;
					for( uint i=0; i<query.size(); ++i )
					{
						if( query[i]=='#' )
							for( ; i<query.size() && query[i]!='\n'; ++i );
						os.put( query[i] );
					}
					_pDataSource->Execute( os.str(), nullptr, nullptr, false );
				}
				DBG( "Finished '{}'"sv, path.string() );
			}
		}
		for( var& [tableName, pTable] : schema.Tables )
		{
			for( var& [id,value] : pTable->FlagsData )
			{
				if( _pDataSource->Scaler<uint>( format("select count(*) from {} where id=?"sv, pTable->Name), {id})==0 )
					_pDataSource->Execute( format("insert into {}(id,name)values( ?, ? )"sv, pTable->Name), {id, value} );
			}
			for( var& jData : pTable->Data )
			{
				vector<object> params;

				ostringstream osSelect{ "select count(*) from ", std::ios::ate }; osSelect << tableName << " where ";
				vector<object> selectParams;
				ostringstream osWhere;
				var set = [&,&table=*pTable]()
				{
					osWhere.str("");
					for( var& keyColumn : table.SurrogateKey )
					{
						if( osWhere.tellp() != std::streampos(0) )
							osWhere << " and ";
						osWhere << keyColumn << "=?";
						if( var pData = jData.find( Schema::ToJson(keyColumn) ); pData!=jData.end() )
							selectParams.push_back( ToObject(table.FindColumn(keyColumn)->Type, *pData, keyColumn) );
						else
						{
							selectParams.clear();
							break;
						}
					}
					return selectParams.size();
				};
				try
				{
					if( !set() )
						for( auto p = pTable->NaturalKeys.begin(); p!=pTable->NaturalKeys.end() && !set(); ++p );
				}
				RETHROW( "Could not set data for {}"sv, tableName );
				THROW_IF( selectParams.empty(), "Could not find keys in data for '{}'", tableName );
				osSelect << osWhere.str();


				ostringstream osInsertValues;
				ostringstream osInsertColumns;
				for( var& column : pTable->Columns )
				{
					var jsonName = Schema::ToJson( column.Name );
					var pData = jData.find( jsonName );
					var haveData = pData!=jData.end();
					if( !haveData && column.Default.empty() )
						continue;

					if( params.size() )
					{
						osInsertValues << ",";
						osInsertColumns << ",";
					}
					osInsertColumns << column.Name;
					if( haveData )
					{
						osInsertValues << "?";
						params.push_back( ToObject(column.Type, *pData, column.Name) );
					}
					else
						osInsertValues << ( column.Default=="$now" ? _syntax.UtcNow() : column.Default );
				}
				if( _pDataSource->Scaler<uint>( osSelect.str(), selectParams)==0 )
				{
					ostringstream sql;
					var haveSequence = pTable->HaveSequence();
					if( haveSequence )
						sql << "SET IDENTITY_INSERT " << tableName << " ON;" << endl;
					sql << format( "insert into {}({})values({})", tableName, osInsertColumns.str(), osInsertValues.str() );
					if( haveSequence )
						sql << endl << "SET IDENTITY_INSERT " << tableName << " OFF;";
					_pDataSource->Execute( sql.str(), params );
				}
			}
		}
		for( auto& [tableName, pTable] : schema.Tables )
		{
			for( auto& column : pTable->Columns )
			{
				if( column.PKTable.empty() )
					continue;
				if( std::find_if(fks.begin(), fks.end(), [&,t=tableName](var& fk){return fk.second.Table==t && fk.second.Columns==vector<string>{column.Name};})!=fks.end() )
					continue;
				var pPKTable = schema.Tables.find( Schema::FromJson(column.PKTable) );
				if( pPKTable == schema.Tables.end() )
				{
					ERR( "Could not find primary key table '{}' for {}.{}"sv, Schema::FromJson(column.PKTable), tableName, column.Name );
					continue;
				}
				if( pPKTable->second->FlagsData.size() )
					column.IsFlags = true;
				else
				{
					if( pPKTable->second->Data.size() )
						column.IsEnum = true;

					auto i = 0;
					auto getName = [&,t=tableName](auto i){ return format( "{}_{}{}_fk", AbbrevName(t), AbbrevName(pPKTable->first), i==0 ? "" : ToString(i)); };
					auto name = getName( i++ );
					for( ; fks.find(name)!=fks.end(); name = getName(i++) );

					var createStatement = ForeignKey::Create( name, column.Name, *pPKTable->second, tableName );
					_pDataSource->Execute( createStatement );
					DBG( "Created fk '{}'."sv, name );
				}
			}
		}
		return schema;
	}

	string UniqueIndexName( const DB::Index& index, ISchemaProc& /*dbSchema*/, bool uniqueName, const vector<Index>& indexes )noexcept(false)
	{
		auto indexName=index.Name;
		bool checkOnlyTable = !index.PrimaryKey && !uniqueName;
		for( uint i=2; ; indexName = format( "{}{}", index.Name, i++ ) )
		{
			if( std::find_if(indexes.begin(), indexes.end(), [&](var& x){ return x.Name==CIString(indexName) && (!checkOnlyTable || index.TableName==x.TableName);})==indexes.end() )
				break;
		}
		return indexName;
	}
}