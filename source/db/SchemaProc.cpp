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
namespace Jde::DB{
	using boost::container::flat_map;
	using nlohmann::json;
	using nlohmann::ordered_json;
	struct IDataSource;

	static sp<LogTag> _logTag{ Logging::Tag("sql") };
	constexpr ELogTags _tags{ ELogTags::Sql };
	α UniqueIndexName( const Index& index, ISchemaProc& dbSchema, bool uniqueName, const vector<Index>& indexes )ε->string;

	string AbbrevName( sv schemaName ){
		auto fnctn = []( var& word )->string{
			ostringstream os;
			for( var ch : word ){
				if( (ch!='a' && ch!='e' && ch!='i' && ch!='o' && ch!='u') || os.tellp() == std::streampos(0) )
					os << ch;
			}
			return word.size()>2 && os.str().size()<word.size()-1 ? os.str() : string{ word };
		};
		var singular = DB::Schema::ToSingular( schemaName );
		var splits = Str::Split( singular, '_' );
		ostringstream name;
		for( uint i=1; i<splits.size(); ++i ){
			if( i>1 )
				name << '_';
			name << fnctn( splits[i] );
		}
		return name.str();
	}
#define _syntax DB::DefaultSyntax()
	α ISchemaProc::CreateSchema( const ordered_json& j, const fs::path& relativePath )ε->Schema{
		var dbTables = LoadTables();

		auto dbIndexes = LoadIndexes();
		auto fks = LoadForeignKeys();
		var procedures = LoadProcs();
		flat_map<string, Table> parentTables;
		Schema schema;
		if( j.contains("$types") ){
			for( var& [columnName, column] : j.find("$types")->items() )
				schema.Types.emplace( columnName, Column{columnName, column, schema.Types, parentTables, j} );
		}
		function<void(sv,const json&)> addTable;
		addTable = [&]( sv key, const json& item ){
			var parentId = item.contains("parent") ? item.find("parent")->get<string>() : string{};
			if( parentId.size() && parentTables.find(parentId)==parentTables.end() ){
				THROW_IF( j.find(parentId)==j.end(), "Could not find '{}' parent {}", key, parentId );
				addTable( parentId, *j.find(parentId) );
			}
			parentTables.emplace( key, Table{key, item, parentTables, schema.Types, j} );
		};
		for( var& [key, value] : j.items() ){
			if( var name = Schema::FromJson(key); !key.starts_with("$") )
				schema.Tables.emplace( name, make_shared<Table>(name, value, parentTables, schema.Types, j) );
			else if( key!="$types" && key!="$scripts" && parentTables.find(key)==parentTables.end() )
				addTable( key, value );
		}
		for( var& [tableName, pTable] : schema.Tables ){
			if( var pNameDBTable=dbTables.find(tableName); pNameDBTable!=dbTables.end() ){
				for( auto& column : pTable->Columns ){
					var& dbTable = pNameDBTable->second;
					var pDBColumn = dbTable.FindColumn( column.Name ); if( !pDBColumn ){ ERR("Could not find db column {}.{}", tableName, column.Name); continue; }
					if( pDBColumn->Default.empty() && column.Default.size() && column.Default!="$now" )
						_pDataSource->TryExecute( _syntax.AddDefault(tableName, column.Name, column.Default) );
				}
			}
			else if( !pTable->IsView ){
				var v = pTable->Create( _syntax );
				_pDataSource->Execute( v );
				INFO( "Created table '{}'."sv, tableName );
				if( pTable->HaveSequence() ){
					for( auto pTableIndex : LoadIndexes({}, pTable->Name) )
						dbIndexes.push_back( pTableIndex );
				}
			}

			for( var& index : pTable->Indexes ){
				if( find_if(dbIndexes.begin(), dbIndexes.end(), [&](var& db){ return db.TableName==index.TableName && db.Columns==index.Columns;} )!=dbIndexes.end() )
					continue;
				var name = UniqueIndexName( index, *this, _syntax.UniqueIndexNames(), dbIndexes );
				var indexCreate = index.Create( name, tableName, _syntax );
				_pDataSource->Execute( indexCreate );
				dbIndexes.push_back( Index{name, tableName, index} );
				INFO( "Created index '{}.{}'."sv, tableName, name );
			}
			if( var procName = pTable->InsertProcName(); procName.size() && procedures.find(procName)==procedures.end() ){
				_pDataSource->Execute( pTable->InsertProcText(_syntax) );
				INFO( "Created proc '{}'."sv, pTable->InsertProcName() );
			}
		}
		if( j.contains("$scripts") ){
			for( var& nameObj : *j.find("$scripts") ){
				auto name = fs::path{ nameObj.get<string>() };
				var procName = name.stem();
				if( procedures.find(procName.string())!=procedures.end() )
					continue;
				if( _syntax.ProcFileSuffix().size() )
					name = name.parent_path()/( name.stem().string()+string{_syntax.ProcFileSuffix()}+name.extension().string() );
				var path = relativePath/name; CHECK_PATH( path, SRCE_CUR );
				var text = IO::FileUtilities::Load( path );
				Trace( _tags, "Executing '{}'", path.string() );
				var queries = Str::Split<sv,iv>( text, "\ngo"_iv );
				for( var& query : queries ){
					ostringstream os;
					for( uint i=0; i<query.size(); ++i ){
						if( query[i]=='#' )
							for( ; i<query.size() && query[i]!='\n'; ++i );
						if( i<query.size() )
							os.put( query[i] );
					}
					_pDataSource->Execute( os.str(), nullptr, nullptr, false ); //TODONext - add views for app server.
				}
				INFO( "Finished '{}'"sv, path.string() );
			}
		}
		for( var& [tableName, pTable] : schema.Tables ){
			for( var& [id,value] : pTable->FlagsData ){
				if( _pDataSource->Scaler<uint>( Jde::format("select count(*) from {} where id=?", tableName), {id})==0 )
					_pDataSource->Execute( Jde::format("insert into {}(id,name)values( ?, ? )", tableName), {id, value} );
			}
			for( var& jData : pTable->Data ){
				vector<object> params;

				ostringstream osSelect{ "select count(*) from ", std::ios::ate }; osSelect << tableName << " where ";
				vector<object> selectParams;
				ostringstream osWhere;
				var set = [&,&table=*pTable](){
					osWhere.str("");
					for( var& keyColumn : table.SurrogateKeys ){
						if( osWhere.tellp() != std::streampos(0) )
							osWhere << " and ";
						osWhere << keyColumn << "=?";
						if( var pData = jData.find( Schema::ToJson(keyColumn) ); pData!=jData.end() )
							selectParams.push_back( ToObject(table.FindColumn(keyColumn)->Type, *pData, keyColumn) );
						else{
							selectParams.clear();
							break;
						}
					}
					return selectParams.size();
				};
				try{
					if( !set() )
						for( auto p = pTable->NaturalKeys.begin(); p!=pTable->NaturalKeys.end() && !set(); ++p );
				}
				RETHROW( "Could not set data for {}", tableName );
				THROW_IF( selectParams.empty(), "Could not find keys in data for '{}'", tableName );
				osSelect << osWhere.str();

				ostringstream osInsertValues;
				ostringstream osInsertColumns;
				for( var& column : pTable->Columns ){
					var jsonName = Schema::ToJson( column.Name );
					var pData = jData.find( jsonName );
					var haveData = pData!=jData.end();
					if( !haveData && column.Default.empty() )
						continue;

					if( params.size() ){
						osInsertValues << ",";
						osInsertColumns << ",";
					}
					osInsertColumns << column.Name;
					if( haveData ){
						osInsertValues << "?";
						params.push_back( ToObject(column.Type, *pData, column.Name) );
					}
					else
						osInsertValues << ( column.Default=="$now" ? ToStr(_syntax.UtcNow()) : column.Default );
				}
				if( _pDataSource->Scaler<uint>( osSelect.str(), selectParams)==0 ){
					ostringstream sql;
					var identityInsert = pTable->HaveSequence() && _syntax.NeedsIdentityInsert();
					if( identityInsert )
						sql << "SET IDENTITY_INSERT " << tableName << " ON;" << std::endl;
					sql << Jde::format( "insert into {}({})values({})", tableName, osInsertColumns.str(), osInsertValues.str() );
					if( identityInsert )
						sql << std::endl << "SET IDENTITY_INSERT " << tableName << " OFF;";
					_pDataSource->Execute( sql.str(), params );
				}
			}
		}
		for( auto& [tableName, pTable] : schema.Tables ){
			for( auto& column : pTable->Columns ){
				if( column.PKTable.empty() )
					continue;
				if( find_if(fks, [&,t=tableName](var& fk){return fk.second.Table==t && fk.second.Columns==vector<string>{column.Name};})!=fks.end() )
					continue;
				var pPKTable = schema.Tables.find( Schema::FromJson(column.PKTable) );
				if( pPKTable == schema.Tables.end() ){
					ERR( "Could not find primary key table '{}' for {}.{}", Schema::FromJson(column.PKTable), tableName, column.Name );
					continue;
				}
				if( pPKTable->second->FlagsData.size() )
					column.IsFlags = true;
				else{
					if( pPKTable->second->Data.size() )
						column.IsEnum = true;

					auto i = 0;
					auto getName = [&, &t=tableName](auto i)->string{//&t for clang
						return Jde::format( "{}_{}{}_fk", AbbrevName(t), AbbrevName(pPKTable->first), i==0 ? "" : ToString(i) );
					};
					auto name = getName( i++ );
					for( ; fks.find(name)!=fks.end(); name = getName(i++) );

					var createStatement = ForeignKey::Create( name, column.Name, *pPKTable->second, tableName );
					fks.emplace( name, ForeignKey{name, tableName, {column.Name}, pPKTable->first} );
					_pDataSource->Execute( createStatement );
					INFO( "Created fk '{}'."sv, name );
				}
			}
		}
		return schema;
	}

	α UniqueIndexName( const DB::Index& index, ISchemaProc& /*dbSchema*/, bool uniqueName, const vector<Index>& indexes )ε->string{
		auto indexName=index.Name;
		bool checkOnlyTable = !index.PrimaryKey && !uniqueName;
		for( uint i=2; ; indexName = Jde::format( "{}{}", index.Name, i++ ) ){
			if( std::find_if(indexes.begin(), indexes.end(), [&](var& x){ return x.Name==CIString(indexName) && (!checkOnlyTable || index.TableName==x.TableName);})==indexes.end() )
				break;
		}
		return indexName;
	}
}