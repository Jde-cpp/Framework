﻿#include "GraphQL.h"
#include "SchemaProc.h"
#include "Row.h"
#include "Database.h"
#include "DataSource.h"
#include <jde/Exception.h>
#include "Syntax.h"
#include "../DateTime.h"
#include "GraphQuery.h"
#include <jde/Str.h>

#define var const auto
namespace Jde
{
	using nlohmann::json;
	using DB::EObject;
	using DB::EType;
	sp<DB::IDataSource> _pDataSource;
#define  _schema DB::DefaultSchema()
	static sp<LogTag> _logTag = Logging::Tag( "ql" );

	α DB::SetQLDataSource( sp<DB::IDataSource> p )ι->void{ _pDataSource = p; }

	α DB::ClearQLDataSource()ι->void{ _pDataSource = nullptr; }
	up<flat_map<string,vector<function<void(const DB::MutationQL& m, PK id)>>>> _applyMutationListeners;  shared_mutex _applyMutationListenerMutex;
	α ApplyMutationListeners()ι->flat_map<string,vector<function<void(const DB::MutationQL& m, PK id)>>>&{ if( !_applyMutationListeners ) _applyMutationListeners = mu<flat_map<string,vector<function<void(const DB::MutationQL& m, PK id)>>>>(); return *_applyMutationListeners; }

	α DB::AddMutationListener( string tablePrefix, function<void(const DB::MutationQL& m, PK id)> listener )ι->void
	{
		unique_lock l{_applyMutationListenerMutex};
		auto& listeners = ApplyMutationListeners().try_emplace( move(tablePrefix) ).first->second;
		listeners.push_back( listener );
	}

	namespace DB::GraphQL
	{
		α DataSource()ι->sp<IDataSource>{ return _pDataSource; }
	}
	α DB::AppendQLSchema( const DB::Schema& schema )ι->void
	{
		for( var& [name,v] : schema.Types )
			_schema.Types.emplace( name, v );
		for( var& [name,v] : schema.Tables )
			_schema.Tables.emplace( name, v );
	}
namespace DB
{
	α MutationQL::TableSuffix()Ι->string
	{
		if( _tableSuffix.empty() )
			_tableSuffix = DB::Schema::ToPlural<string>( DB::Schema::FromJson<string,string>(JsonName) );
		return _tableSuffix;
	}

	α MutationQL::InputParam( sv name )Ε->json
	{
		var input = Input();
		var p = input.find( name ); THROW_IF( p==input.end(), "Could not find '{}' argument. {}", name, input.dump() );
		return *p;
	}
	α MutationQL::Input()Ε->json
	{
		var pInput = Args.find("input"); THROW_IF( pInput==Args.end(), "Could not find input argument. {}", Args.dump() );
		return *pInput;
	}

	α TableQL::DBName()Ι->string
	{
		return DB::Schema::ToPlural( DB::Schema::FromJson(JsonName) );
	}
}
#define TEST_ACCESS(a,b,c) TRACE( "TEST_ACCESS({},{},{})"sv, a, b, c )
	α ToJsonName( DB::Column c )ε
	{
		string tableName;
		auto memberName{ DB::Schema::ToJson(c.Name) };
		if( c.IsFlags || c.IsEnum )
		{
			var pPKTable = _schema.Tables.find( c.PKTable ); THROW_IF( pPKTable==_schema.Tables.end(), "can not find column {}'s pk table {}.", c.Name, c.PKTable );
			tableName = pPKTable->second->Name;
			memberName = DB::Schema::ToJson( pPKTable->second->NameWithoutType() );
			if( c.IsEnum )
				memberName = DB::Schema::ToSingular( memberName );
		}
		return make_tuple( memberName, tableName );
	}
	α Insert( const DB::Table& table, const DB::MutationQL& m )ε->uint
	{
		ostringstream sql{ string{DB::Schema::ToSingular(table.Name)}, std::ios::ate }; sql << "_insert(";
		vector<DB::object> parameters; parameters.reserve( table.Columns.size() );
		var pInput = m.Args.find( "input" ); THROW_IF( pInput == m.Args.end(), "Could not find 'input' arg." );
		for( var& c : table.Columns )
		{
			if( !c.Insertable )
				continue;
			var [memberName, tableName] = ToJsonName( c );
			var pValue = pInput->find( memberName );

			var exists = pValue!=pInput->end(); THROW_IF( !exists && !c.IsNullable && c.Default.empty() && c.Insertable, "No value specified for {}.", memberName );
			if( parameters.size() )
				sql << ", ";
			sql << "?";
			DB::object value;
			if( !exists )
				value = c.Default.size() ? DB::object{ c.Default } : DB::object{ nullptr };
			else
			{
				if( c.IsEnum && pValue->is_string() )
				{
					var pValues = SFuture<flat_map<uint,string>>( _pDataSource->SelectEnum<uint>(tableName) ).get();
					optional<uint> pEnum = FindKey( *pValues, pValue->get<string>() ); THROW_IF( !pEnum, "Could not find '{}' for {}", pValue->get<string>(), memberName );
					value = *pEnum;
				}
				else
					value = ToObject( c.Type, *pValue, memberName );
			}
			parameters.push_back( value );
		}
		sql << ")";
		uint id;
		auto result = [&id]( const DB::IRow& row ){ id = (int32)row.GetInt(0); };
		_pDataSource->ExecuteProc( sql.str(), parameters, result );
		return id;
	}
	//um_permissions
	α SubWhere( const DB::Table& table, const DB::Column& c, vector<DB::object>& params, uint paramIndex )ε->string
	{
		ostringstream sql{ "=( select id from ", std::ios::ate }; sql << table.Name << " where " << c.Name;
		if( c.QLAppend.size() )
		{
			CHECK( paramIndex<params.size() && params[paramIndex].index()==(uint)EObject::String );
			var split = Str::Split( get<string>(params.back()), "\\" ); CHECK( split.size() );
			var appendColumnName = DB::Schema::FromJson( c.QLAppend );
			var pColumn = table.FindColumn( appendColumnName ); CHECK( pColumn ); CHECK( pColumn->PKTable.size() );
			sql << (split.size()==1 ? " is null" : "=?") << " and " << appendColumnName << "=(select id from " <<  pColumn->PKTable << " where name=?) )";
			if( split.size()>1 )
			{
				params.push_back( split[1] );
				params[paramIndex] = split[0];
			}
		}
		else
			sql << "=? )";
		return sql.str();
	}

	α Update( const DB::Table& table, const DB::MutationQL& m, const DB::Syntax& syntax )->uint
	{
		ostringstream sql{ Jde::format("update {} set ", table.Name), std::ios::ate };
		vector<DB::object> parameters; parameters.reserve( table.Columns.size() );
		var pInput = m.Args.find( "input" ); THROW_IF( pInput==m.Args.end(), "Could not find input argument. {}", m.Args.dump() );
		string sqlUpdate;
		vector<string> where; where.reserve(2);
		vector<DB::object> whereParams; whereParams.reserve( 2 );
		for( var& c : table.Columns )
		{
			if( !c.Updateable )
			{
				if( var p=find(table.SurrogateKey.begin(), table.SurrogateKey.end(), c.Name); p!=table.SurrogateKey.end() )
				{
					if( var pId = m.Args.find( DB::Schema::ToJson(c.Name) ); pId!=m.Args.end() )
					{
						whereParams.push_back( ToObject(EType::ULong, *pId, c.Name) );
						where.push_back( c.Name+"=?" );
					}
					else
					{
						auto pTable = c.Name==table.ParentId() ? table.ParentTable( _schema ) : table.ChildTable( _schema ); CHECK( pTable );
						auto pValue = m.Args.find( "target" );
						sv cName{ pValue==m.Args.end() ? "name" : "target" };
						if( pValue==m.Args.end() )
						{
							pValue = m.Args.find( cName ); CHECK( pValue!=m.Args.end() );
						}
						var pNameColumn = pTable->FindColumn( cName ); CHECK( pNameColumn );
						whereParams.push_back( pValue->get<string>() );
						where.push_back( c.Name+SubWhere(*pTable, *pNameColumn, whereParams, 1) );
					}
					//else
					//	THROW( "Could not get criterial from {}", m.Args.dump() );
				}
				else if( c.Name=="updated" )
					sqlUpdate = Jde::format( ",{}={}", c.Name, ToSV(syntax.UtcNow()) );
			}
			else
			{
				var [memberName, tableName] = ToJsonName( c );
				var pValue = pInput->find( memberName );
				if( pValue==pInput->end() )
					continue;
				if( parameters.size() )
					sql << ", ";
				sql << c.Name << "=?";
				if( c.IsFlags )
				{
					uint value = 0;
					if( pValue->is_array() && pValue->size() )
					{
						auto a = _pDataSource->SelectMap<string,uint>( Jde::format("select name, id from {}", tableName) );
						var values = Future<flat_map<string,uint>>( move(a) ).get();
						for( var& flag : *pValue )
						{
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
		THROW_IF( where.size()==0, "There is no where clause." );
		THROW_IF( parameters.size()==0, "There is nothing to update." );
		for( var& param : whereParams ) parameters.push_back( param );
		if( sqlUpdate.size() )
			sql << sqlUpdate;
		sql << " where " << Str::AddSeparators( where, " and " );

		var result = _pDataSource->Execute( sql.str(), parameters );
//		DBG( "result={}"sv, result );
		return result;
	}
	α Delete( const DB::Table& table, const DB::MutationQL& m )->uint
	{
		var pId = m.Args.find( "id" ); THROW_IF( pId==m.Args.end(), "Could not find id argument. {}", m.Args.dump() );
		var pColumn = find_if( table.Columns.begin(), table.Columns.end(), []( var& c ){ return c.Name=="deleted"; } ); THROW_IF( pColumn==table.Columns.end(), "Could not find 'deleted' column" );
		vector<DB::object> parameters;
		parameters.push_back( ToObject(EType::ULong, *pId, "id") );
		ostringstream sql{ "update ", std::ios::ate }; sql << table.Name << " set deleted=" << DB::DefaultSyntax().UtcNow() << " where id=?";
		return _pDataSource->Execute( sql.str(), parameters );
	}
	α Restore( const DB::Table& table, const DB::MutationQL& m )->uint
	{
		var pId = m.Args.find( "id" ); THROW_IF( pId==m.Args.end(), "Could not find id argument. {}", m.Args.dump() );
		var pColumn = find_if( table.Columns.begin(), table.Columns.end(), []( var& c ){ return c.Name=="deleted"; } ); THROW_IF( pColumn==table.Columns.end(), "Could not find 'deleted' column" );
		vector<DB::object> parameters;
		parameters.push_back( ToObject(EType::ULong, *pId, "id") );
		ostringstream sql{ "update ", std::ios::ate }; sql << table.Name << " set deleted=null where id=?";
		return _pDataSource->Execute( sql.str(), parameters );
	}
	α Purge( sv tableName, const DB::MutationQL& m, UserPK userId, SRCE )ε->uint
	{
		var pId = m.Args.find( "id" ); THROW_IF( pId==m.Args.end(), "Could not find id argument. {}", m.Args.dump() );
		vector<DB::object> parameters;
		parameters.push_back( ToObject(EType::ULong, *pId, "id") );
		if( var p=UM::FindAuthorizer(tableName); p )
			p->TestPurge( *pId, userId );
		return _pDataSource->Execute( Jde::format("delete from {} where id=?", tableName), parameters, sl );
	}

	α ChildParentParams( sv childId, sv parentId, const json& input )->vector<DB::object>
	{
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
	α Add( const DB::Table& table, const json& input )->uint
	{
		ostringstream sql{ "insert into ", std::ios::ate }; sql << table.Name << "(" << table.ChildId() << "," << table.ParentId();
		var childId = DB::Schema::ToJson( table.ChildId() );
		var parentId = DB::Schema::ToJson( table.ParentId() );
		auto parameters = ChildParentParams( childId, parentId, input );
		ostringstream values{ "?,?", std::ios::ate };
		for( var& [name,value] : input.items() )
		{
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
	α Remove( const DB::Table& table, const json& input )->uint
	{
		auto params = ChildParentParams( DB::Schema::ToJson(table.ChildId()), DB::Schema::ToJson(table.ParentId()), input );
		ostringstream sql{ "delete from ", std::ios::ate }; sql << table.Name << " where " << table.ChildId();
		if( (EObject)params[0].index()==EObject::String )
		{
			var pTable = table.ChildTable(_schema); CHECK( pTable );
			var pTarget = pTable->FindColumn( "target" ); CHECK( pTarget );
			sql << SubWhere( *pTable, *pTarget, params, 0 );
		}
		else
			sql << "=?";
		sql << " and " << table.ParentId();
		if( (EObject)params[1].index()==EObject::String )
		{
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

	α Mutation( const DB::MutationQL& m, UserPK userId )ε->uint
	{
		var& t = _schema.FindTableSuffix( m.TableSuffix() );
		if( var pAuthorizer = UM::FindAuthorizer(t.Name); pAuthorizer )
			pAuthorizer->Test( m.Type, userId );
		uint result;
		if( m.Type==DB::EMutationQL::Create )
			result = Insert( t, m );
		else if( m.Type==DB::EMutationQL::Update )
			result = Update( t, m, DB::DefaultSyntax() );
		else if( m.Type==DB::EMutationQL::Delete )
			result = Delete( t, m );
		else if( m.Type==DB::EMutationQL::Restore )
			result = Restore( t, m );
		else if( m.Type==DB::EMutationQL::Purge )
			result = Purge( t.Name, m, userId );
		else
		{
			if( m.Type==DB::EMutationQL::Add )
				result = Add( t, m.Input() );
			else if( m.Type==DB::EMutationQL::Remove )
				result = Remove( t, m.Input() );
			else
				THROW( "unknown type" );
		}
		if( t.Name.starts_with("um_") )
			Try( [&]{UM::ApplyMutation( m, (UserPK)result );} );
		sl _{ _applyMutationListenerMutex };
		if( var p = ApplyMutationListeners().find(string{t.Prefix()}); p!=ApplyMutationListeners().end() )
			std::for_each( p->second.begin(), p->second.end(), [&](var& f){f(m, result);} );

		return result;
	}

	α DB::ColumnQL::QLType( const DB::Column& column, SL sl )ε->string
	{
		string qlTypeName = "ID";
		if( !column.IsId )
		{
			switch( column.Type )
			{
			case EType::Bit:
				qlTypeName = "Boolean";
				break;
			case EType::Int16: case EType::Int: case EType::Int8: case EType::Long:
				qlTypeName = "Int";
				break;
			case EType::UInt16: case EType::UInt: case EType::ULong:
				qlTypeName = "UInt";
				break;
			case EType::SmallFloat: case EType::Float: case EType::Decimal: case EType::Numeric: case EType::Money:
				qlTypeName = "Float";
				break;
			case EType::None: case EType::Binary: case EType::VarBinary: case EType::Guid: case EType::Cursor: case EType::RefCursor: case EType::Image: case EType::Blob: case EType::TimeSpan:
				throw Exception{ sl, ELogLevel::Debug, "EType {} is not implemented.", (uint)column.Type };
			case EType::VarTChar: case EType::VarWChar: case EType::VarChar: case EType::NText: case EType::Text: case EType::Uri:
				qlTypeName = "String";
				break;
			case EType::TChar: case EType::WChar: case EType::UInt8: case EType::Char:
				qlTypeName = "Char";
			case EType::DateTime: case EType::SmallDateTime:
				qlTypeName = "DateTime";
				break;
			}
		}
		return qlTypeName;
	}
	α IntrospectFields( sv typeName, const DB::Table& dbTable, const DB::TableQL& fieldTable, json& jData )ε
	{
		auto fields = json::array();
		var pTypeTable = fieldTable.FindTable( "type" );
		var haveName = fieldTable.FindColumn( "name" )!=nullptr;
		var pOfTypeTable = pTypeTable->FindTable( "ofType" );
		auto addField = [&]( sv name, sv typeName, DB::QLFieldKind typeKind, sv ofTypeName, optional<DB::QLFieldKind> ofTypeKind )
		{
			json field;
			if( haveName )
				field["name"] = name;
			if( pTypeTable )
			{
				json type;
				auto setField = []( const DB::TableQL& t, json& j, str key, sv x ){ if( t.FindColumn(key) ){ if(x.size()) j[key]=x; else j[key]=nullptr; } };
				auto setKind = []( const DB::TableQL& t, json& j, optional<DB::QLFieldKind> pKind )
				{
					if( t.FindColumn("kind") )
					{
						if( pKind )
							j["kind"] = (uint)*pKind;
						else
							j["kind"] = nullptr;
					}
				};
				setField( *pTypeTable, type, "name", typeName );
				setKind( *pTypeTable, type, typeKind );
				if( pOfTypeTable && (ofTypeName.size() || ofTypeKind) )
				{
					json ofType;
					setField( *pOfTypeTable, ofType, "name", ofTypeName );
					setKind( *pOfTypeTable, ofType, ofTypeKind );
					type["ofType"] = ofType;
				}
				field["type"] = type;
			}
			fields.push_back( field );
		};
		function<void(const DB::Table&, bool, string)> addColumns = [&addColumns,&addField,&typeName]( const DB::Table& dbTable2, bool isMap, string prefix={} )
		{
			for( var& column : dbTable2.Columns )
			{
				string fieldName;
				string qlTypeName;
				DB::QLFieldKind rootType = DB::QLFieldKind::Scalar;
				if( column.PKTable.empty() )
				{
					if( prefix.size() && column.Name=="id" )//use NID see RolePermission's permissions
						continue;
					fieldName = DB::Schema::ToJson( column.Name );
					qlTypeName = DB::ColumnQL::QLType( column );//column.PKTable.empty() ? DB::ColumnQL::QLType( column ) : dbTable.JsonTypeName();
				}
				else if( var pPKTable=_schema.Tables.find(column.PKTable); pPKTable!=_schema.Tables.end() )
				{
					if( !isMap || pPKTable->second->IsFlags() ) //!RolePermission || right_id
					{
						if( std::find_if(dbTable2.Columns.begin(), dbTable2.Columns.end(), [&column]( var& c ){return c.QLAppend==DB::Schema::ToJson(column.Name);})!=dbTable2.Columns.end() )
							continue;
						rootType = pPKTable->second->IsEnum() ? DB::QLFieldKind::Enum : DB::QLFieldKind::Object;
						qlTypeName = pPKTable->second->JsonTypeName();
						fieldName = DB::Schema::ToJson<sv>( qlTypeName );
						if( pPKTable->second->IsFlags() )
						{
							fieldName = DB::Schema::ToPlural<sv>( fieldName );
							rootType = DB::QLFieldKind::List;
						}
					}
					else //isMap
					{
						if( !typeName.starts_with(pPKTable->second->JsonTypeName()) )//typeName==RolePermission, don't want role columns, just permissions.
							addColumns( *pPKTable->second, false, pPKTable->second->JsonTypeName() );
						continue;
					}
				}
				else
					THROW( "Could not find table '{}'", column.PKTable );

				var isNullable = column.IsNullable;
				string typeName2 = isNullable ? qlTypeName : "";
				var typeKind = isNullable ? rootType : DB::QLFieldKind::NonNull;
				string ofTypeName = isNullable ? "" : qlTypeName;
				var ofTypeKind = isNullable ? optional<DB::QLFieldKind>{} : rootType;

				addField( fieldName, typeName2, typeKind, ofTypeName, ofTypeKind );
			}
		};
		addColumns( dbTable, dbTable.IsMap(), {} );
		for( var& [name,pTable] : _schema.Tables )
		{
			auto fnctn = [addField,p=pTable,&dbTable]( var& c1Name, var& c2Name )
			{
				if( var pColumn1=p->FindColumn(c1Name), pColumn2=p->FindColumn(c2Name) ; pColumn1 && pColumn2 /*&& pColumn->PKTable==n*/ )
				{
					if( pColumn1->PKTable==dbTable.Name )
					{
						var jsonType = p->Columns.size()==2 ? _schema.Tables[pColumn2->PKTable]->JsonTypeName() : p->JsonTypeName();
						addField( DB::Schema::ToPlural<string>(DB::Schema::ToJson<sv>(jsonType)), {}, DB::QLFieldKind::List, jsonType, DB::QLFieldKind::Object );
					}
				}
			};
			var child = pTable->ChildId();
			var parent = pTable->ParentId();
			if( child.size() && parent.size() )
			{
				fnctn( child, parent );
				fnctn( parent, child );
			}
		}
		json jTable;
		jTable["fields"] = fields;
		jTable["name"] = dbTable.JsonTypeName();
		jData["__type"] = jTable;
	}
	α IntrospectEnum( sv /*typeName*/, const DB::Table& dbTable, const DB::TableQL& fieldTable, json& jData )ε
	{
		auto fields = json::array();
		ostringstream sql{ "select ", std::ios::ate };
		vector<string> columns;
		for_each( fieldTable.Columns.begin(), fieldTable.Columns.end(), [&columns, &dbTable](var& x){ if(dbTable.FindColumn(x.JsonName)) columns.push_back(x.JsonName);} );//sb only id/name.
		sql << Str::AddCommas( columns ) << " from " << dbTable.Name << " order by id";
		_pDataSource->Select( sql.str(), [&]( const DB::IRow& row )
		{
			json j;
			for( uint i=0; i<columns.size(); ++i )
			{
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
	α QueryType( const DB::TableQL& typeTable, json& jData )ε
	{
		THROW_IF( typeTable.Args.find("name")==typeTable.Args.end(), "__type data for all names '{}' not supported", typeTable.JsonName );
		var typeName = typeTable.Args["name"].get<string>();
		auto p = std::find_if( _schema.Tables.begin(), _schema.Tables.end(), [&](var& t){ return t.second->JsonTypeName()==typeName;} ); THROW_IF( p==_schema.Tables.end(), "Could not find table '{}' in schema", typeName );
		for( var& pQlTable : typeTable.Tables )
		{
			if( pQlTable->JsonName=="fields" )
				IntrospectFields( typeName,  *p->second, *pQlTable, jData );
			else if( pQlTable->JsonName=="enumValues" )
				IntrospectEnum( typeName,  *p->second, *pQlTable, jData );
			else
				THROW( "__type data for '{}' not supported", pQlTable->JsonName );
		}
	}
	α QuerySchema( const DB::TableQL& schemaTable, json& jData )ε->void
	{
		THROW_IF( schemaTable.Tables.size()!=1, "Only Expected 1 table type for __schema {}", schemaTable.Tables.size() );
		var pMutationTable = schemaTable.Tables[0]; THROW_IF( pMutationTable->JsonName!="mutationType", "Only mutationType implemented for __schema - {}", pMutationTable->JsonName );
		auto fields = json::array();
		for( var& nameTablePtr : _schema.Tables )
		{
			var pDBTable = nameTablePtr.second;
			var childId = pDBTable->ChildId();
			var jsonType = pDBTable->JsonTypeName();

			json field;
			field["name"] = Jde::format( "create{}"sv, jsonType );
			var addField = [&jsonType, pDBTable, &fields]( sv name, bool allColumns=false, bool idColumn=true )
			{
				json field;
				auto args = json::array();
				for( var& column : pDBTable->Columns )
				{
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
			if( childId.empty() )
			{
				addField( "insert", true, false );
				addField( "update", true );

				addField( "delete" );
				addField( "restore" );
				addField( "purge" );
			}
			else
			{
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
	α QueryTable( const DB::TableQL& table, UserPK userId, json& jData )ε->void
	{
		TEST_ACCESS( "Read", table.DBName(), userId ); //TODO implement.
		if( table.JsonName=="__type" )
			QueryType( table, jData );
		else if( table.JsonName=="__schema" )
			QuerySchema( table, jData );
		else
			DB::GraphQL::Query( table, jData, userId );
	}

	α QueryTables( const vector<DB::TableQL>& tables, UserPK userId )ε->json
	{
		json data;
		for( var& table : tables )
			QueryTable( table, userId, data["data"] );
		return data;
	}

	α DB::CoQuery( string&& q_, UserPK u_, str threadName, SL sl )ι->TPoolAwait<json>
	{
		return Coroutine::TPoolAwait<json>( [q=move(q_), u=u_](){ return mu<json>(Query(q,u)); }, threadName, sl );
	}
	α DB::Query( sv query, UserPK userId )ε->json
	{
		TRACE( "{}", query );
		var qlType = ParseQL( query );
		vector<DB::TableQL> tableQueries;
		json j;
		if( qlType.index()==1 )
		{
			var& mutation = get<MutationQL>( qlType );
			uint id = Mutation( mutation, userId );
			if( mutation.ResultPtr && mutation.ResultPtr->Columns.size()==1 && mutation.ResultPtr->Columns.front().JsonName=="id" )
				j["data"][mutation.JsonName]["id"] = id;
			else if( mutation.ResultPtr )
				tableQueries.push_back( *mutation.ResultPtr );
		}
		else
			tableQueries = get<vector<DB::TableQL>>( qlType );
		json y = tableQueries.size() ? QueryTables( tableQueries, userId ) : j;
		//Dbg( y.dump() );
		return y;
	}
	struct Parser2
	{
		Parser2( sv text, sv delimiters )ι: _text{text}, Delimiters{delimiters}{}
		α Next()ι->sv
		{
			sv result = _peekValue;
			if( result.empty() )
			{
				if( i<_text.size() )
					for( auto ch = _text[i]; i<_text.size() && isspace(ch); ch = i<_text.size()-1 ? _text[++i] : _text[i++] );
				if( i<_text.size() )
				{
					uint start=i;
					i = start+std::distance( _text.begin()+i, std::find_if(_text.begin()+i, _text.end(), [this]( char ch )ι{ return isspace(ch) || Delimiters.find(ch)!=sv::npos;}) );
					result = i==start ? _text.substr( i++, 1 ) : _text.substr( start, i-start );
				}
			}
			else
				_peekValue = {};
			return result;
		};
		sv Next( char end )ι
		{
			sv result;
			if( _peekValue.size() )
			{
				i = i-_peekValue.size();
				_peekValue = {};
			}
			for( auto ch = _text[i]; i<_text.size() && isspace(ch); ch = _text[++i] );
			if( i<_text.size() )
			{
				uint start = i;
				for( auto ch = _text[i]; i<_text.size() && ch!=end; ch = _text[++i] );
				++i;
				result = _text.substr( start, i-start );
			}
			return result;
		};
		sv Peek()ι{ return _peekValue.empty() ? _peekValue = Next() : _peekValue; }
		uint Index()ι{ return i;}
		sv Text()ι{ return i<_text.size() ? _text.substr(i) : sv{}; }
		sv AllText()ι{ return _text; }
	private:
		uint i{0};
		sv _text;
		sv Delimiters;
		sv _peekValue;
	};
	α StringifyKeys( sv json )->string
	{
		ostringstream os;
		bool inValue = false;
		for( uint i=0; i<json.size(); ++i )
		{
			char ch = json[i];
			if( std::isspace(ch) )
				os << ch;
			else if( ch=='{' || ch=='}' )
			{
				os << ch;
				inValue = false;
			}
			else if( inValue )
			{
				if( ch=='{' )
				{
					for( uint i2=i+1, openCount=1; i2<json.size(); ++i2 )
					{
						if( json[i2]=='{' )
							++openCount;
						else if( json[i2]=='}' && --openCount==0 )
						{
							os << StringifyKeys( json.substr(i,i2-i) );
							i = i2+1;
							break;
						}
					}
				}
				else if( ch=='[' )
				{
					for( ; i<json.size() && ch!=']'; ch = json[++i] )
						os << ch;
					os << ']';
				}
				else
				{
					for( ; i<json.size() && ch!=',' && ch!='}'; ch = json[++i] )
						os << ch;
					if( i<json.size() )
						os << ch;
				}
				inValue = false;
			}
			else
			{
				var quoted = ch=='"';
				if( !quoted ) --i;
				for( ch = '"'; i<json.size() && ch!=':'; ch=json[++i] )
					os << ch;
				if( !quoted )
					os << '"';
				os << ":";
				inValue = true;
			}
		}
		return os.str();
	}
	α ParseJson( Parser2& q )->json
	{
		string params{ q.Next(')') }; THROW_IF( params.front()!='(', "Expected '(' vs {} @ '{}' to start function - '{}'.",  params.front(), q.Index()-1, q.Text() );
		params.front()='{'; params.back() = '}';
		params = StringifyKeys( params );
		return json::parse( params );
	}
	α LoadTable( Parser2& q, sv jsonName )ε->DB::TableQL//__type(name: "Account") { fields { name type { name kind ofType{name kind} } } }
	{
		var j = q.Peek()=="(" ? ParseJson( q ) : json{};
		THROW_IF( q.Next()!="{", "Expected '{{' after table name. i='{}', text='{}'", q.Index(), q.AllText() );
		DB::TableQL table{ string{jsonName}, j };
		for( auto token = q.Next(); token!="}" && token.size(); token = q.Next() )
		{
			if( q.Peek()=="{" )
				table.Tables.push_back( make_shared<DB::TableQL>(LoadTable(q, token)) );
			else
				table.Columns.emplace_back( DB::ColumnQL{string{token}} );
		}
		return table;
	}
	α LoadTables( Parser2& q, sv jsonName )ε->vector<DB::TableQL>
	{
		vector<DB::TableQL> results;
		do
		{
			results.push_back( LoadTable(q, jsonName) );
			jsonName = {};
			if( q.Peek()=="," )
			{
				q.Next();
				jsonName = q.Next();
			}
		}while( jsonName.size() );
		return results;
	}

	α LoadMutation( Parser2& q )->DB::MutationQL
	{
		THROW_IF( q.Next()!="{", "mutation expected '{' as 1st character." );
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
	α DB::ParseQL( sv query )ε->DB::RequestQL
	{
		uint i = query.find_first_of( "{" ); THROW_IF( i==sv::npos || i>query.size()-2, "Invalid query '{}'", query );
		Parser2 parser{ query.substr(i+1), "{}()," };
		auto name = parser.Next();
		if( name=="query" )
			name = parser.Next();

		return name=="mutation" ? DB::RequestQL{ LoadMutation(parser) } : DB::RequestQL{ LoadTables(parser,name) };
	}
}