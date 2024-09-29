#include "GraphQL.h"
#include <jde/Exception.h>
#include <jde/Str.h>
#include <jde/io/Json.h>
#include <jde/db/graphQL/GraphQLHook.h>
#include "graphQL/Introspection.h"
#include "SchemaProc.h"
#include "Row.h"
#include "Database.h"
#include "DataSource.h"
#include "Syntax.h"
#include "../DateTime.h"
#include "GraphQuery.h"
#include "types/WhereClause.h"
#include "graphQL/Insert.h"
#include "graphQL/Purge.h"

#define var const auto
namespace Jde{
	using nlohmann::json;
	using DB::EObject;
	using DB::EType;
	using DB::GraphQL::QLFieldKind;
#define	_pDataSource DB::DataSourcePtr()
#define  _schema DB::DefaultSchema()
	up<DB::GraphQL::Introspection> _introspection;
	constexpr ELogTags _tags{ ELogTags::GraphQL };
	constexpr ELogTags _parsingTags{ ELogTags::GraphQL | ELogTags::Parsing };

	α DB::SetQLDataSource( sp<DB::IDataSource> p )ι->void{ _pDataSource = p; }

	α DB::ClearQLDataSource()ι->void{ _pDataSource = nullptr; }
	//MutationListeners - deprecated, use hooks.
	up<flat_map<string,vector<function<void(const DB::MutationQL& m, PK id)>>>> _applyMutationListeners;  shared_mutex _applyMutationListenerMutex;
	α ApplyMutationListeners()ι->flat_map<string,vector<function<void(const DB::MutationQL& m, PK id)>>>&{ if( !_applyMutationListeners ) _applyMutationListeners = mu<flat_map<string,vector<function<void(const DB::MutationQL& m, PK id)>>>>(); return *_applyMutationListeners; }

	α DB::AddMutationListener( string tablePrefix, function<void(const DB::MutationQL& m, PK id)> listener )ι->void{
		unique_lock l{_applyMutationListenerMutex};
		auto& listeners = ApplyMutationListeners().try_emplace( move(tablePrefix) ).first->second;
		listeners.push_back( listener );
	}

	namespace DB{
		α GraphQL::DataSource()ι->sp<IDataSource>{ return _pDataSource; }
	}
	α DB::AppendQLDBSchema( const DB::Schema& schema )ι->void{
		for( var& [name,v] : schema.Types )
			_schema.Types.emplace( name, v );
		for( var& [name,v] : schema.Tables )
			_schema.Tables.emplace( name, v );
	}

	α DB::SetQLIntrospection( json&& j )ε->void{
		_introspection = mu<GraphQL::Introspection>( move(j) );
	}

namespace DB{
	α GraphQL::ToJsonName( DB::Column c )ε->tuple<string,string>{
		string tableName;
		auto memberName{ DB::Schema::ToJson(c.Name) };
		if( c.IsFlags || c.IsEnum ){
			var pkTable = _schema.FindTable( c.PKTable );
			tableName = pkTable.Name;
			memberName = DB::Schema::ToJson( pkTable.NameWithoutType() );
			if( c.IsEnum )
				memberName = DB::Schema::ToSingular( memberName );
		}
		return make_tuple( memberName, tableName );
	}
}
	using DB::GraphQL::ToJsonName;
	using DB::GraphQL::InsertAwait;
	//um_permissions
	α SubWhere( const DB::Table& table, const DB::Column& c, vector<DB::object>& params, uint paramIndex )ε->string{
		ostringstream sql{ "=( select id from ", std::ios::ate }; sql << table.Name << " where " << c.Name;
		if( c.QLAppend.size() ){
			CHECK( paramIndex<params.size() && params[paramIndex].index()==(uint)EObject::String );
			var split = Str::Split( get<string>(params.back()), "\\" ); CHECK( split.size() );
			var appendColumnName = DB::Schema::FromJson( c.QLAppend );
			var pColumn = table.FindColumn( appendColumnName ); CHECK( pColumn ); CHECK( pColumn->PKTable.size() );
			sql << (split.size()==1 ? " is null" : "=?") << " and " << appendColumnName << "=(select id from " <<  pColumn->PKTable << " where name=?) )";
			if( split.size()>1 ){
				params.push_back( split[1] );
				params[paramIndex] = split[0];
			}
		}
		else
			sql << "=? )";
		return sql.str();
	}

	α Update( const DB::Table& table, const DB::MutationQL& m, const DB::Syntax& syntax )->tuple<uint,DB::object>{
		var pExtendedFromTable = table.GetExtendedFromTable( _schema );
		auto [count,rowId] = pExtendedFromTable ? Update(*pExtendedFromTable, m, syntax) : make_tuple( 0, 0 );
		ostringstream sql{ Jde::format("update {} set ", table.Name), std::ios::ate };
		vector<DB::object> parameters; parameters.reserve( table.Columns.size() );
		var input = m.Input();
		string sqlUpdate;
		DB::WhereClause where;
		for( var& c : table.Columns ){
			if( !c.Updateable ){
				if( var p=find(table.SurrogateKeys, c.Name); p!=table.SurrogateKeys.end() ){
					if( pExtendedFromTable )
						where.Add( c.Name, rowId );
					else if( var pId = m.Args.find( DB::Schema::ToJson(c.Name) ); pId!=m.Args.end() )
						where.Add( c.Name, rowId=ToObject(EType::ULong, *pId, c.Name) );
					else{
						auto parentColumnName = table.ParentColumn() ? table.ParentColumn()->Name : string{};
						auto pTable = c.Name==parentColumnName ? table.ParentTable( _schema ) : table.ChildTable( _schema ); CHECK( pTable );
						auto pValue = m.Args.find( "target" );
						sv cName{ pValue==m.Args.end() ? "name" : "target" };
						if( pValue==m.Args.end() ){
							pValue = m.Args.find( cName ); CHECK( pValue!=m.Args.end() );
						}
						var pNameColumn = pTable->FindColumn( cName ); CHECK( pNameColumn );
						auto whereParams = where.Parameters();
						whereParams.push_back( pValue->get<string>() );
						where << c.Name+SubWhere( *pTable, *pNameColumn, whereParams, whereParams.size()-1 );
					}
					//else
					//	THROW( "Could not get criterial from {}", m.Args.dump() );
				}
				else if( c.Name=="updated" )
					sqlUpdate = Jde::format( ",{}={}", c.Name, ToSV(syntax.UtcNow()) );
			}
			else{
				var [memberName, tableName] = ToJsonName( c );
				var pValue = input.find( memberName );
				if( pValue==input.end() )
					continue;
				if( parameters.size() )
					sql << ", ";
				sql << c.Name << "=?";
				if( c.IsFlags ){
					uint value = 0;
					if( pValue->is_array() && pValue->size() ){
						optional<flat_map<string,uint>> values;
						[] (auto& values, auto& tableName)ι->Task{ 
							values = *( co_await _pDataSource->SelectMap<string,uint>(Jde::format("select name, id from {}", tableName)) ).UP<flat_map<string,uint>>(); 
						}(values, tableName);
						while( !values )
							std::this_thread::yield();

						for( var& flag : *pValue ){
							if( var pFlag = values->find(flag); pFlag != values->end() )
								value |= pFlag->second;
						}
					}
					parameters.push_back( value );
				}
				else
					parameters.push_back( ToObject(c.Type, *pValue, memberName) );
			}
		}
		THROW_IF( where.Size()==0, "There is no where clause." );
		THROW_IF( parameters.size()==0 && count==0, "There is nothing to update." );
		uint result{};
		if( parameters.size() ){
			for( var& param : where.Parameters() ) parameters.push_back( param );
			if( sqlUpdate.size() )
				sql << sqlUpdate;
			sql << ' ' << where.Move();

			result = _pDataSource->Execute( sql.str(), parameters );
		}
		return make_tuple( count+result, rowId );
	}
	α SetDeleted( const DB::Table& table, uint id, iv value )ε->uint{
		var column = table.FindColumn( "deleted", _schema );
		vector<DB::object> parameters{ id };
		const string sql{ Jde::format("update {} set deleted={} where id=?", column.TablePtr->Name, value) };
		return _pDataSource->Execute( sql, parameters );
	}
	α Delete( const DB::Table& table, const DB::MutationQL& m )ε->uint{
		return SetDeleted( table, m.Id(), DB::DefaultSyntax().UtcNow() );
	}
	α Restore( const DB::Table& table, const DB::MutationQL& m )ε->uint{
		return SetDeleted( table, m.Id(), "null" );
	}

	α ChildParentParams( sv childId, sv parentId, const json& input )->vector<DB::object>{
		vector<DB::object> parameters;
		if( var p = input.find(childId); p!=input.end() )
			parameters.push_back( ToObject(EType::ULong, *p, childId) );
		else if( var p = input.find("target"); p!=input.end() )
			parameters.push_back( p->get<string>() );
		else
			THROW( "Could not find '{}' or target in '{}'", childId, input.dump() );

		if( var p = input.find(parentId); p!=input.end() )
			parameters.push_back( ToObject(EType::ULong, *p, parentId) );
		else if( var p = input.find("name"); p!=input.end() )
			parameters.push_back( p->get<string>() );
		else
			THROW( "Could not find '{}' or name in '{}'", parentId, input.dump() );

		return parameters;
	};
	α Add( const DB::Table& table, const json& input )->uint{
		string childColName = table.ChildColumn()->Name;
		string parentColName = table.ParentColumn()->Name;
		ostringstream sql{ "insert into ", std::ios::ate }; sql << table.Name << "(" << childColName << "," << parentColName;
		var childId = DB::Schema::ToJson( childColName );
		var parentId = DB::Schema::ToJson( parentColName );
		auto parameters = ChildParentParams( childId, parentId, input );
		ostringstream values{ "?,?", std::ios::ate };
		for( var& [name,value] : input.items() ){
			if( name==childId || name==parentId )
				continue;
			var columnName = DB::Schema::FromJson( name );
			auto pColumn = table.FindColumn( columnName ); THROW_IF(!pColumn, "Could not find column {}.{}.", table.Name, columnName );
			sql << "," << columnName;
			values << ",?";

			parameters.push_back( ToObject(pColumn->Type, value, name) );
		}
		sql << ")values( " << values.str() << ")";
		return _pDataSource->Execute( sql.str(), parameters );
	}
	α Remove( const DB::Table& table, const json& input )->uint{
		var& childId = table.ChildColumn()->Name;
		var& parentId = table.ParentColumn()->Name;
		auto params = ChildParentParams( DB::Schema::ToJson(childId), DB::Schema::ToJson(parentId), input );
		ostringstream sql{ "delete from ", std::ios::ate }; sql << table.Name << " where " << childId;
		if( (EObject)params[0].index()==EObject::String ){
			var pTable = table.ChildTable(_schema); CHECK( pTable );
			var pTarget = pTable->FindColumn( "target" ); CHECK( pTarget );
			sql << SubWhere( *pTable, *pTarget, params, 0 );
		}
		else
			sql << "=?";
		sql << " and " << parentId;
		if( (EObject)params[1].index()==EObject::String ){
			var pTable = table.ParentTable(_schema); CHECK( pTable );
			var pName = pTable->FindColumn( "name" ); CHECK( pName );
			sql << SubWhere( *pTable, *pName, params, 1 );
/*			sql << "=( select id from " << pTable->Name << " where name";
			if( pName->QLAppend.size() )
			{

				var split = Str::Split( get<string>(params[1]), "\\" ); CHECK( split.size() );
				var appendColumnName = DB::Schema::FromJson( pName->QLAppend );
				var pColumn = pTable->FindColumn( appendColumnName ); CHECK( pColumn ); CHECK( pColumn->PKTable.size() );
				sql << (split.size()==1 ? " is null" : "=?") << " and " << appendColumnName << "=(select id from " <<  pColumn->PKTable << " where name=?) )";
				if( split.size()>1 )
				{
					params.push_back( split[1] );
					params[1] = split[0];
				}
			}
			else
				sql << "=? )";*/
		}
		else
			sql << "=?";

		return _pDataSource->Execute( sql.str(), params );
	}

	α Start( sp<DB::MutationQL> m, UserPK userPK )ε->uint{
		optional<uint> result;
		[&]()ι->DB::GraphQL::MutationAwaits::Task {
			auto await_ = DB::GraphQL::Hook::Start( m, userPK );
			result = co_await await_;
		}();
		while( !result )
			std::this_thread::yield();//TODO remove this when async.
		return *result;
	}

	α Stop( sp<DB::MutationQL> m, UserPK userPK )ε->uint{
		optional<uint> result;
		[&]()ι->DB::GraphQL::MutationAwaits::Task {
			result = co_await DB::GraphQL::Hook::Stop( m, userPK );
		}();
	while( !result )
			std::this_thread::yield();//TODO remove this when async.
		return *result;
	}
#pragma warning( disable : 4701 )
	α Mutation( const DB::MutationQL& m, UserPK userPK )ε->uint{
		var& table = _schema.FindTableSuffix( m.TableSuffix() );
		if( var pAuthorizer = UM::FindAuthorizer(table.Name); pAuthorizer )
			pAuthorizer->Test( m.Type, userPK );
		optional<uint> result;
		switch( m.Type ){
		using namespace DB;
		case EMutationQL::Create:{
			optional<uint> extendedFromId;
			if( var pExtendedFrom = table.GetExtendedFromTable(_schema); pExtendedFrom ){
				[](auto& extendedFromId, var& pExtendedFrom, var& m, auto userPK)ι->Task{
					extendedFromId = *( co_await InsertAwait(*pExtendedFrom, m, userPK, 0) ).UP<uint>();
				}(extendedFromId, pExtendedFrom, m, userPK);
				while (!extendedFromId)
					std::this_thread::yield();
			}
			auto _tag = ELogTags::GraphQL | ELogTags::Pedantic;
			Trace{ _tag, "calling InsertAwait" };
			[] (auto& table, auto& m, auto userPK, auto extendedFromId, auto& result)ι->Task{
				result = *( co_await InsertAwait(table, m, userPK, extendedFromId.value_or(0)) ).UP<uint>();
			}(table, m, userPK, extendedFromId, result);
			while (!result)
				std::this_thread::yield();
			if( extendedFromId )
				result = extendedFromId;
			Trace{ _tag, "~calling InsertAwait" };
		break;}
		case EMutationQL::Update:
			result = get<0>( Update(table, m, DB::DefaultSyntax()) );
			break;
		case EMutationQL::Delete:
			result = Delete( table, m );
			break;
		case EMutationQL::Restore:
			result = Restore( table, m );
			break;
		case EMutationQL::Purge:
			[] (auto& table, auto& m, auto userPK, auto& result)ι->Task{
				result = *( co_await DB::GraphQL::PurgeAwait{table, m, userPK} ).UP<uint>();
			}( table, m, userPK, result );
			while (!result)
				std::this_thread::yield();
			break;
		case EMutationQL::Add:
			result = Add( table, m.Input() );
			break;
		case EMutationQL::Remove:
			result = Remove( table, m.Input() );
			break;
		case EMutationQL::Start:
			Start( ms<DB::MutationQL>(m), userPK );
			result = 1;
			break;
		case EMutationQL::Stop:
			Stop( ms<DB::MutationQL>(m), userPK );
			result = 1;
			break;
//		default:
//			THROW( "unknown type" );
		}
		if( table.Name.starts_with("um_") )
			Try( [&]{UM::ApplyMutation( m, (UserPK)result.value_or(0) );} );
		sl _{ _applyMutationListenerMutex };
		//deprecated, use hooks
		if( var p = ApplyMutationListeners().find(string{table.Prefix()}); p!=ApplyMutationListeners().end() )
			std::for_each( p->second.begin(), p->second.end(), [&](var& f){f(m, result.value_or(0));} );

		return *result;
	}

	α IntrospectFields( sv /*typeName*/, const DB::Table& mainTable, const DB::TableQL& fieldTable, json& jData )ε{
		auto fields = json::array();
		var pTypeTable = fieldTable.FindTable( "type" );
		var haveName = fieldTable.FindColumn( "name" )!=nullptr;
		var pOfTypeTable = pTypeTable->FindTable( "ofType" );
		json jTable;
		jTable["name"] = mainTable.JsonTypeName();

		auto addField = [&]( sv name, sv typeName, QLFieldKind typeKind, sv ofTypeName, optional<QLFieldKind> ofTypeKind ){
			json field;
			if( haveName )
				field["name"] = name;
			if( pTypeTable ){
				json type;
				auto setField = []( const DB::TableQL& t, json& j, str key, sv x ){ if( t.FindColumn(key) ){ if(x.size()) j[key]=x; else j[key]=nullptr; } };
				auto setKind = []( const DB::TableQL& t, json& j, optional<QLFieldKind> pKind ){
					if( t.FindColumn("kind") ){
						if( pKind )
							j["kind"] = (uint)*pKind;
						else
							j["kind"] = nullptr;
					}
				};
				setField( *pTypeTable, type, "name", typeName );
				setKind( *pTypeTable, type, typeKind );
				if( pOfTypeTable && (ofTypeName.size() || ofTypeKind) ){
					json ofType;
					setField( *pOfTypeTable, ofType, "name", ofTypeName );
					setKind( *pOfTypeTable, ofType, ofTypeKind );
					type["ofType"] = ofType;
				}
				field["type"] = type;
			}
			fields.push_back( field );
		};
		function<void(const DB::Table&, bool, string)> addColumns = [&addColumns,&addField,&mainTable]( const DB::Table& dbTable, bool isMap, string prefix={} ){
			for( var& column : dbTable.Columns ){
				BREAK_IF( column.Name=="member_id" );
				string fieldName;
				string qlTypeName;
				auto rootType{ QLFieldKind::Scalar };
				if( column.PKTable.empty() ){
					if( prefix.size() && column.Name=="id" )//use NID see RolePermission's permissions
						continue;
					fieldName = DB::Schema::ToJson( column.Name );
					qlTypeName = DB::ColumnQL::QLType( column );//column.PKTable.empty() ? DB::ColumnQL::QLType( column ) : dbTable.JsonTypeName();
				}
				else if( var pPKTable=_schema.TryFindTable(column.PKTable); pPKTable ){
					auto pChildColumn = dbTable.ChildColumn();
					if( !isMap || pPKTable->IsFlags() || (pChildColumn && pChildColumn->Name==column.Name)  ){ //!RolePermission || right_id || um_groups.member_id
						if( find_if(dbTable.Columns, [&column](var& c){return c.QLAppend==DB::Schema::ToJson(column.Name);})!=dbTable.Columns.end() )
							continue;
						if( auto pExtendedFromTable = mainTable.GetExtendedFromTable(_schema); pExtendedFromTable && mainTable.SurrogateKey().Name==column.Name ){//extension table
							addColumns( *pPKTable, false, prefix );
							continue;
						}
						qlTypeName = pPKTable->JsonTypeName();
						if( pPKTable->IsFlags() ){
							fieldName = DB::Schema::ToPlural<sv>( fieldName );
							rootType = QLFieldKind::List;
						}
						else if( pChildColumn ){
							fieldName = DB::Schema::ToPlural<sv>( DB::Schema::ToJson(Str::Replace(column.Name, "_id", "")) );
							rootType = QLFieldKind::List;
						}
						else{
							fieldName = DB::Schema::ToJson<sv>( qlTypeName );
							rootType = pPKTable->IsEnum() ? QLFieldKind::Enum : QLFieldKind::Object;
						}
					}
					else{ //isMap
						//if( !typeName.starts_with(pPKTable->JsonTypeName()) )//typeName==RolePermission, don't want role columns, just permissions.
						addColumns( *pPKTable, false, pPKTable->JsonTypeName() );
						continue;
					}
				}
				else
					THROW( "Could not find table '{}'", column.PKTable );

				auto pChildColumn = dbTable.ChildColumn();//group.member_id may not exist.
				var isNullable = pChildColumn || column.IsNullable;
				var typeName2 = isNullable ? qlTypeName : "";
				var typeKind = isNullable ? rootType : QLFieldKind::NonNull;
				var ofTypeName = isNullable ? "" : qlTypeName;
				var ofTypeKind = isNullable ? optional<QLFieldKind>{} : rootType;

				addField( fieldName, typeName2, typeKind, ofTypeName, ofTypeKind );
			}
		};
		addColumns( mainTable, mainTable.IsMap(_schema), {} );
		for( var& [name,pTable] : _schema.Tables ){
			auto fnctn = [addField,p=pTable,&mainTable]( var& c1Name, var& c2Name ){
				if( var pColumn1=p->FindColumn(c1Name), pColumn2=p->FindColumn(c2Name) ; pColumn1 && pColumn2 /*&& pColumn->PKTable==n*/ ){
					if( pColumn1->PKTable==mainTable.Name ){
						var jsonType = p->Columns.size()==2 ? _schema.Tables[pColumn2->PKTable]->JsonTypeName() : p->JsonTypeName();
						addField( DB::Schema::ToPlural<string>(DB::Schema::ToJson<sv>(jsonType)), {}, QLFieldKind::List, jsonType, QLFieldKind::Object );
					}
				}
			};
			var child = pTable->ChildColumn();
			var parent = pTable->ParentColumn();
			if( child && parent ){
				fnctn( child->Name, parent->Name );
				fnctn( parent->Name, child->Name );
			}
		}
		jTable["fields"] = fields;
		jData["__type"] = jTable;
	}
	α IntrospectEnum( sv /*typeName*/, const DB::Table& baseTable, const DB::TableQL& fieldTable, json& jData )ε{
		var& dbTable = baseTable.QLView.empty() ? baseTable : _schema.FindTable( baseTable.QLView );
		ostringstream sql{ "select ", std::ios::ate };
		vector<string> columns;
		for_each( fieldTable.Columns, [&columns, &dbTable](var& x){ if(dbTable.FindColumn(x.JsonName)) columns.push_back(x.JsonName);} );//sb only id/name.
		sql << Str::AddCommas( columns ) << " from " << dbTable.Name << " order by id";
		auto fields = json::array();
		_pDataSource->Select( sql.str(), [&]( const DB::IRow& row ){
			json j;
			for( uint i=0; i<columns.size(); ++i ){
				if( columns[i]=="id" )
					j[columns[i]] = row.GetUInt( i );
				else
					j[columns[i]] = row.GetString( i );
			}
			fields.push_back( j );
		} );
		json jTable;
		jTable["enumValues"] = fields;
		jData["__type"] = jTable;
	}
	α QueryType( const DB::TableQL& typeTable, json& jData )ε{
		THROW_IF( typeTable.Args.find("name")==typeTable.Args.end(), "__type data for all names '{}' not supported", typeTable.JsonName );
		var typeName = typeTable.Args["name"].get<string>();
		auto p = find_if( _schema.Tables, [&](var& t){ return t.second->JsonTypeName()==typeName;} ); THROW_IF( p==_schema.Tables.end(), "Could not find table '{}' in schema", typeName );
		for( var& qlTable : typeTable.Tables ){
			if( qlTable.JsonName=="fields" ){
				if( var pObject = _introspection ? _introspection->Find(typeName) : nullptr; pObject )
					jData["__type"] = pObject->ToJson( qlTable );
				else
					IntrospectFields( typeName,  *p->second, qlTable, jData );
			}
			else if( qlTable.JsonName=="enumValues" )
				IntrospectEnum( typeName,  *p->second, qlTable, jData );
			else
				THROW( "__type data for '{}' not supported", qlTable.JsonName );
		}
	}
	α QuerySchema( const DB::TableQL& schemaTable, json& jData )ε->void{
		THROW_IF( schemaTable.Tables.size()!=1, "Only Expected 1 table type for __schema {}", schemaTable.Tables.size() );
		var& mutationTable = schemaTable.Tables[0]; THROW_IF( mutationTable.JsonName!="mutationType", "Only mutationType implemented for __schema - {}", mutationTable.JsonName );
		auto fields = json::array();
		for( var& nameTablePtr : _schema.Tables ){
			var pDBTable = nameTablePtr.second;
			var childColumn = pDBTable->ChildColumn();
			var jsonType = pDBTable->JsonTypeName();

			json field;
			field["name"] = Jde::format( "create{}"sv, jsonType );
			var addField = [&jsonType, pDBTable, &fields]( sv name, bool allColumns=false, bool idColumn=true ){
				json field;
				auto args = json::array();
				for( var& column : pDBTable->Columns ){
					if(   (column.Name=="id" && !idColumn) || (column.Name!="id" && !allColumns) )
						continue;
					json arg;
					arg["name"] = DB::Schema::ToJson( column.Name );
					arg["defaultValue"] = nullptr;
					json type; type["name"] = DB::ColumnQL::QLType( column );
					arg["type"]=type;
					args.push_back( arg );
				}
				field["args"] = args;
				field["name"] = Jde::format( "{}{}", name, jsonType );
				fields.push_back( field );
			};
			if( !childColumn ){
				addField( "insert", true, false );
				addField( "update", true );

				addField( "delete" );
				addField( "restore" );
				addField( "purge" );
			}
			else{
				addField( "add", true, false );
				addField( "remove", true, false );
			}
		}
		json jmutationType;
		jmutationType["fields"] = fields;
		jmutationType["name"] = "Mutation";
		json jSchema; jSchema["mutationType"] = jmutationType;
		jData["__schema"] = jmutationType;
	}
#define TEST_ACCESS(a,b,c) Trace( _tags, "TEST_ACCESS({},{},{})", a, b, c )
	α QueryTable( const DB::TableQL& table, UserPK userPK, json& jData )ε->void{
		TEST_ACCESS( "Read", table.DBName(), userPK ); //TODO implement.
		if( table.JsonName=="__type" )
			QueryType( table, jData );
		else if( table.JsonName=="__schema" )
			QuerySchema( table, jData );
		else
			DB::GraphQL::Query( table, jData, userPK );
	}

	α QueryTables( const vector<DB::TableQL>& tables, UserPK userPK )ε->json{
		json data;
		for( var& table : tables )
			QueryTable( table, userPK, data["data"] );
		return data;
	}

	α DB::CoQuery( string q_, UserPK u_, str threadName, SL sl )ι->TPoolAwait<json>{
		return Coroutine::TPoolAwait<json>( [q=move(q_), u=u_](){
			return mu<json>(Query(q,u));
		}, threadName, sl );
	}

	α DB::Query( sv query, UserPK userPK )ε->json{
		ASSERT( _pDataSource );
		Trace( _tags, "{}", query );
		try{
			var qlType = ParseQL( query );
			vector<DB::TableQL> tableQueries;
			json j;
			if( qlType.index()==1 ){
				var& mutation = get<MutationQL>( qlType );
				uint result = Mutation( mutation, userPK );
				str resultMemberName = mutation.Type==EMutationQL::Create ? "id" : "rowCount";
				var wantResults = mutation.ResultPtr && mutation.ResultPtr->Columns.size()>0;
				if( wantResults && mutation.ResultPtr->Columns.front().JsonName==resultMemberName )
					j["data"][mutation.JsonName][resultMemberName] = result;
				else if( mutation.ResultPtr )
					tableQueries.push_back( *mutation.ResultPtr );
			}
			else
				tableQueries = get<vector<DB::TableQL>>( qlType );
			json y = tableQueries.size() ? QueryTables( tableQueries, userPK ) : j;
			//Dbg( y.dump() );
			return y;
		}
		catch( const nlohmann::json::exception& e ){
			THROW( "Error parsing query '{}'.  '{}'", query, e.what() );
		}
	}
	struct Parser2{
		Parser2( sv text, sv delimiters )ι: _text{text}, Delimiters{delimiters}{}
		α Next()ι->sv{
			sv result = _peekValue;
			if( result.empty() ){
				if( i<_text.size() )
					for( auto ch = _text[i]; i<_text.size() && isspace(ch); ch = i<_text.size()-1 ? _text[++i] : _text[i++] );
				if( i<_text.size() ){
					uint start=i;
					i = start+std::distance( _text.begin()+i, std::find_if(_text.begin()+i, _text.end(), [this]( char ch )ι{ return isspace(ch) || Delimiters.find(ch)!=sv::npos;}) );
					result = i==start ? _text.substr( i++, 1 ) : _text.substr( start, i-start );
				}
			}
			else
				_peekValue = {};
			return result;
		};
		sv Next( char end )ι{
			sv result;
			if( _peekValue.size() ){
				i = i-_peekValue.size();
				_peekValue = {};
			}
			for( auto ch = _text[i]; i<_text.size() && isspace(ch); ch = _text[++i] );
			if( i<_text.size() ){
				uint start = i;
				for( auto ch = _text[i]; i<_text.size() && ch!=end; ch = _text[++i] );
				++i;
				result = _text.substr( start, i-start );
			}
			return result;
		};
		sv Peek()ι{ return _peekValue.empty() ? _peekValue = Next() : _peekValue; }
		uint Index()ι{ return i; }
		sv Text()ι{ return i<_text.size() ? _text.substr(i) : sv{}; }
		sv AllText()ι{ return _text; }
	private:
		uint i{0};
		sv _text;
		sv Delimiters;
		sv _peekValue;
	};

	α StringifyKeys( sv json )ι->string{
		string y{}; y.reserve( json.size()*2 );
		bool inValue = false;
		for( uint i=0; i<json.size(); ++i ){
			char ch = json[i];
			if( std::isspace(ch) || ch==',' )
				y += ch;
			else if( ch=='{' || ch=='}' ){
				y += ch;
				inValue = false;
			}
			else if( inValue ){
				if( ch=='{' ){
					for( uint i2=i+1, openCount=1; i2<json.size(); ++i2 ){
						if( json[i2]=='{' )
							++openCount;
						else if( json[i2]=='}' && --openCount==0 ){
							y += StringifyKeys( json.substr(i,i2-i) );
							i = i2+1;
							break;
						}
					}
				}
				else if( ch=='[' ){
					for( ; i<json.size() && ch!=']'; ch = json[++i] )
						y += ch;
					y += ']';
				}
				else{
					for( ; i<json.size() && ch!=',' && ch!='}'; ch = json[++i] )
						y += ch;
					if( i<json.size() )
						y += ch;
				}
				inValue = false;
			}
			else{
				var quoted = ch=='"';
				if( !quoted ) --i;
				for( ch = '"'; i<json.size() && ch!=':'; ch=json[++i] )
					y += ch;
				if( !quoted )
					y += '"';
				y += ":";
				inValue = true;
			}
		}
		return y;
	}
	α ParseJson( Parser2& q )ε->json{
		string params{ q.Next(')') }; THROW_IF( params.front()!='(', "Expected '(' vs {} @ '{}' to start function - '{}'.",  params.front(), q.Index()-1, q.Text() );
		params.front()='{'; params.back() = '}';
		params = StringifyKeys( params );
		return Json::Parse( params );
	}
	α LoadTable( Parser2& q, sv jsonName )ε->DB::TableQL{//__type(name: "Account") { fields { name type { name kind ofType{name kind} } } }
		var j = q.Peek()=="(" ? ParseJson( q ) : json{};
		if( q.Next()!="{" )
			throw Exception{ _parsingTags, SRCE_CUR, "Expected '{{' after table name. i='{}', text='{}'", q.Index(), q.AllText() };

		DB::TableQL table{ string{jsonName}, j };
		for( auto token = q.Next(); token!="}" && token.size(); token = q.Next() ){
			if( q.Peek()=="{" )
				table.Tables.push_back( LoadTable(q, token) );
			else
				table.Columns.emplace_back( DB::ColumnQL{string{token}} );
		}
		return table;
	}
	α LoadTables( Parser2& q, sv jsonName )ε->vector<DB::TableQL>{
		vector<DB::TableQL> results;
		do{
			results.push_back( LoadTable(q, jsonName) );
			jsonName = {};
			if( q.Peek()=="," ){
				q.Next();
				jsonName = q.Next();
			}
		}while( jsonName.size() );
		return results;
	}

	α LoadMutation( Parser2& q )->DB::MutationQL{
		if( q.Peek()=="{" )
			throw Exception{ _parsingTags, SRCE_CUR, "mutation not expecting '{{' as 1st character." };
		var command = q.Next();
		uint iType=0;
		for( ;iType<DB::MutationQLStrings.size() && !command.starts_with(DB::MutationQLStrings[iType]); ++iType );
		THROW_IF( iType==DB::MutationQLStrings.size(), "Could not find mutation {}", command );

		auto tableJsonName = string{ command.substr(DB::MutationQLStrings[iType].size()) };
		tableJsonName[0] = (char)tolower( tableJsonName[0] );
		var type = (DB::EMutationQL)iType;
		var j = ParseJson( q );
		var wantsResult  = q.Peek()=="{";
		optional<DB::TableQL> result = wantsResult ? LoadTable( q, tableJsonName ) : optional<DB::TableQL>{};
		DB::MutationQL ql{ string{tableJsonName}, type, j, result/*, parentJsonName*/ };
		return ql;
	}
	α DB::ParseQL( sv query )ε->DB::RequestQL{
		uint i = query.find_first_of( "{" ); THROW_IF( i==sv::npos || i>query.size()-2, "Invalid query '{}'", query );
		Parser2 parser{ query.substr(i+1), "{}()," };
		auto name = parser.Next();
		if( name=="query" )
			name = parser.Next();

		return name=="mutation" ? DB::RequestQL{ LoadMutation(parser) } : DB::RequestQL{ LoadTables(parser,name) };
	}
}