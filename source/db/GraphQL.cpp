#include "GraphQL.h"
#include "SchemaProc.h"
#include "Row.h"
#include "Database.h"
#include "DataSource.h"
#include <jde/Exception.h>
#include "Syntax.h"
#include "../DateTime.h"
#include <jde/Str.h>

#define var const auto
namespace Jde
{
	using nlohmann::json;
	using DB::EDataValue;
	using DB::DataType;
	sp<DB::IDataSource> _pDataSource;
	void DB::SetQLDataSource( sp<DB::IDataSource> p )noexcept{ _pDataSource = p; }
	void DB::ClearQLDataSource()noexcept{ _pDataSource = nullptr; }
	flat_map<string,vector<function<void(const DB::MutationQL& m, PK id)>>> _applyMutationListeners;  shared_mutex _applyMutationListenerMutex;
	void DB::AddMutationListener( sv tablePrefix, function<void(const DB::MutationQL& m, PK id)> listener )noexcept
	{
		unique_lock l{_applyMutationListenerMutex};
		auto& listeners = _applyMutationListeners.try_emplace( string{tablePrefix} ).first->second;
		listeners.push_back( listener );
	}
	DB::Schema _schema;
	void DB::AppendQLSchema( const DB::Schema& schema )noexcept
	{
		for( var& [name,v] : schema.Types )
			_schema.Types.emplace( name, v );
		for( var& [name,v] : schema.Tables )
			_schema.Tables.emplace( name, v );
	}
	nlohmann::json InputParam2( sv name, const nlohmann::json& input )noexcept(false)
	{
		var p = input.find( name ); THROW_IF( p==input.end(), "Could not find '{}' argument. {}", name, input.dump() );
		return *p;
	}
namespace DB
{
	string MutationQL::TableSuffix()const noexcept
	{
		if( _tableSuffix.empty() )
			_tableSuffix = DB::Schema::ToPlural( DB::Schema::FromJson(JsonName) );
		return _tableSuffix;
	}

	nlohmann::json MutationQL::InputParam( sv name )const noexcept(false)
	{
		return InputParam2( name, Input() );
	}
	nlohmann::json MutationQL::Input()const noexcept(false)
	{
		var pInput = Args.find("input"); THROW_IF( pInput==Args.end(), "Could not find input argument. {}", Args.dump() );
		return *pInput;
	}

	string TableQL::DBName()const noexcept
	{
		return DB::Schema::ToPlural( DB::Schema::FromJson(JsonName) );
	}
}
#define TEST_ACCESS(a,b,c) DBG( "TEST_ACCESS({},{},{})"sv, a, b, c )

	uint Insert( const DB::Table& table, const DB::MutationQL& m )noexcept(false)
	{
		ostringstream sql{ DB::Schema::ToSingular(table.Name), std::ios::ate }; sql << "_insert(";
		std::vector<DB::DataValue> parameters; parameters.reserve( table.Columns.size() );
		var pInput = m.Args.find( "input" ); THROW_IF( pInput == m.Args.end(), "Could not find 'input' arg." );
		for( var& column : table.Columns )
		{
			if( !column.Insertable )
				continue;
			var memberName = DB::Schema::ToJson( column.Name );
			var pValue = pInput->find( memberName );

			var exists = pValue!=pInput->end(); THROW_IF( !exists && !column.IsNullable && column.Default.empty() && column.Insertable, "No value specified for {}.", memberName );
			if( parameters.size() )
				sql << ", ";
			sql << "?";
			DB::DataValue value;
			if( !exists )
				value = column.Default.size() ? DB::DataValue{column.Default} : DB::DataValue{nullptr};
			else
				value = ToDataValue( column.Type, *pValue, memberName );
			parameters.push_back( value );
		}
		sql << ")";
		uint id;
		auto result = [&id]( const DB::IRow& row ){ id = (int32)row.GetInt(0); };
		_pDataSource->ExecuteProc( sql.str(), parameters, result );
		return id;
	}
	uint Update( const DB::Table& table, const DB::MutationQL& m, const DB::Syntax& syntax )
	{
		ostringstream sql{ "update ", std::ios::ate }; sql << table.Name << " set ";
		std::vector<DB::DataValue> parameters; parameters.reserve( table.Columns.size() );
		var pInput = m.Args.find( "input" ); THROW_IF( pInput==m.Args.end(), "Could not find input argument. {}", m.Args.dump() );
		string sqlUpdate;
		vector<string> where; where.reserve(2);
		vector<DB::DataValue> whereParams; whereParams.reserve( 2 );
		for( var& column : table.Columns )
		{
			if( !column.Updateable )
			{
				if( var p=find(table.SurrogateKey.begin(), table.SurrogateKey.end(), column.Name); p!=table.SurrogateKey.end() )
				{
					var pId = m.Args.find( DB::Schema::ToJson(column.Name) ); THROW_IF( pId==m.Args.end(), "Could not find '{}' argument. {}", DB::Schema::ToJson(column.Name), m.Args.dump() );
					whereParams.push_back( ToDataValue(DataType::ULong, *pId, column.Name) );
					where.push_back( column.Name+"=?" );
				}
				else if( column.Name=="updated" )
					sqlUpdate = format( ",{}={}", column.Name, syntax.UtcNow() );
			}
			else
			{
				string memberName = DB::Schema::ToJson( column.Name );
				string tableName;
				if( column.IsFlags )
				{
					var pPKTable = _schema.Tables.find( column.PKTable ); THROW_IF( pPKTable==_schema.Tables.end(), "can not find column {}'s pk table {}.", column.Name, column.PKTable );
					tableName = pPKTable->second->Name;
					memberName = DB::Schema::ToJson( pPKTable->second->NameWithoutType() );
				}
				var pValue = pInput->find( memberName );
				if( pValue==pInput->end() )
					continue;
				if( parameters.size() )
					sql << ", ";
				sql << column.Name << "=?";
				if( column.IsFlags )
				{
					uint value = 0;
					if( pValue->is_array() && pValue->size() )
					{
						var values = _pDataSource->SelectMap<string,uint>( format("select name, id from {}", tableName) );
						for( var& flag : *pValue )
						{
							if( var pFlag = values->find(flag); pFlag != values->end() )
								value |= pFlag->second;
						}
					}
					parameters.push_back( value );
				}
				else
					parameters.push_back( ToDataValue(column.Type, *pValue, memberName) );
			}
		}
		THROW_IF( parameters.size()==0, "There is nothing to update." );
		for( var& param : whereParams ) parameters.push_back( param );
		if( sqlUpdate.size() )
			sql << sqlUpdate;
		sql << " where " << Str::AddSeparators( where, " and " );

		var result = _pDataSource->Execute( sql.str(), parameters );
//		DBG( "result={}"sv, result );
		return result;
	}
	uint Delete( const DB::Table& table, const DB::MutationQL& m )
	{
		var pId = m.Args.find( "id" ); THROW_IF( pId==m.Args.end(), "Could not find id argument. {}", m.Args.dump() );
		var pColumn = find_if( table.Columns.begin(), table.Columns.end(), []( var& c ){ return c.Name=="deleted"; } ); THROW_IF( pColumn==table.Columns.end(), "Could not find 'deleted' column" );
		std::vector<DB::DataValue> parameters;
		parameters.push_back( ToDataValue(DataType::ULong, *pId, "id") );
		ostringstream sql{ "update ", std::ios::ate }; sql << table.Name << " set deleted=" << DB::DefaultSyntax()->UtcNow() << " where id=?";
		return _pDataSource->Execute( sql.str(), parameters );
	}
	α Restore( const DB::Table& table, const DB::MutationQL& m )->uint
	{
		var pId = m.Args.find( "id" ); THROW_IF( pId==m.Args.end(), "Could not find id argument. {}", m.Args.dump() );
		var pColumn = find_if( table.Columns.begin(), table.Columns.end(), []( var& c ){ return c.Name=="deleted"; } ); THROW_IF( pColumn==table.Columns.end(), "Could not find 'deleted' column" );
		std::vector<DB::DataValue> parameters;
		parameters.push_back( ToDataValue(DataType::ULong, *pId, "id") );
		ostringstream sql{ "update ", std::ios::ate }; sql << table.Name << " set deleted=null where id=?";
		return _pDataSource->Execute( sql.str(), parameters );
	}
	α Purge( sv tableName, const DB::MutationQL& m )->uint
	{
		var pId = m.Args.find( "id" ); THROW_IF( pId==m.Args.end(), "Could not find id argument. {}", m.Args.dump() );
		std::vector<DB::DataValue> parameters;
		parameters.push_back( ToDataValue(DataType::ULong, *pId, "id") );
		ostringstream sql{ "delete from ", std::ios::ate }; sql << tableName << " where id=?";
		return _pDataSource->Execute( sql.str(), parameters );
	}

	std::vector<DB::DataValue> ChildParentParams( sv childId, sv parentId, const json& input )
	{
		std::vector<DB::DataValue> parameters;
		parameters.push_back( ToDataValue(DataType::ULong, InputParam2(childId, input), childId) );
		parameters.push_back( ToDataValue(DataType::ULong, InputParam2(parentId, input), parentId) );
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

			parameters.push_back( ToDataValue(pColumn->Type, value, name) );
		}
		sql << ")values( " << values.str() << ")";
		return _pDataSource->Execute( sql.str(), parameters );
	}
	α Remove( const DB::Table& table, const json& input )->uint
	{
		ostringstream sql{ "delete from ", std::ios::ate }; sql << table.Name << " where " << table.ChildId() << "=? and " << table.ParentId() << "=?";

		return _pDataSource->Execute( sql.str(), ChildParentParams(DB::Schema::ToJson(table.ChildId()), DB::Schema::ToJson(table.ParentId()), input) );
	}

	α Mutation( const DB::MutationQL& m, UserPK userId )noexcept(false)->uint
	{
		var pSyntax = DB::DefaultSyntax();
		var pSchemaTable = _schema.FindTableSuffix( m.TableSuffix() ); THROW_IF( !pSchemaTable, "Could not find table suffixed with '{}' in schema", m.TableSuffix() );
		TEST_ACCESS( "Write", pSchemaTable->Name, userId ); //TODO implement.
		uint result;
		if( m.Type==DB::EMutationQL::Create )
			result = Insert( *pSchemaTable, m );
		else if( m.Type==DB::EMutationQL::Update )
			result = Update( *pSchemaTable, m, *pSyntax );
		else if( m.Type==DB::EMutationQL::Delete )
			result = Delete( *pSchemaTable, m );
		else if( m.Type==DB::EMutationQL::Restore )
			result = Restore( *pSchemaTable, m );
		else if( m.Type==DB::EMutationQL::Purge )
			result = Purge( pSchemaTable->Name, m );
		else
		{
			if( m.Type==DB::EMutationQL::Add )
				result = Add( *pSchemaTable, m.Input() );
			else if( m.Type==DB::EMutationQL::Remove )
				result = Remove( *pSchemaTable, m.Input() );
			else
				throw "unknown type";
		}
		if( pSchemaTable->Name.starts_with("um_") )
			Try( [&]{UM::ApplyMutation( m, result );} );
		shared_lock l{ _applyMutationListenerMutex };
		if( var p = _applyMutationListeners.find(pSchemaTable->Prefix()); p!=_applyMutationListeners.end() )
			std::for_each( p->second.begin(), p->second.end(), [&](var& f){f(m, result);} );

		return result;
	}

	string DB::ColumnQL::QLType( const DB::Column& column )noexcept
	{
		string qlTypeName = "ID";
		if( !column.IsId )
		{
			switch( column.Type )
			{
			case DataType::Bit:
				qlTypeName = "Boolean";
				break;
			case DataType::Int16:
			case DataType::Int:
			case DataType::Int8:
			case DataType::Long:
				qlTypeName = "Int";
				break;
			case DataType::UInt16:
			case DataType::UInt:
			case DataType::ULong:
				qlTypeName = "UInt";
				break;
			case DataType::SmallFloat:
			case DataType::Float:
			case DataType::Decimal:
			case DataType::Numeric:
			case DataType::Money:
				qlTypeName = "Float";
				break;
			case DataType::None:
			case DataType::Binary:
			case DataType::VarBinary:
			case DataType::Guid:
			case DataType::Cursor:
			case DataType::RefCursor:
			case DataType::Image:
			case DataType::Blob:
			case DataType::TimeSpan:
				THROW( "DataType {} is not implemented.", column.Type );
			case DataType::VarTChar:
			case DataType::VarWChar:
			case DataType::VarChar:
			case DataType::NText:
			case DataType::Text:
			case DataType::Uri:
				qlTypeName = "String";
				break;
			case DataType::TChar:
			case DataType::WChar:
			case DataType::UInt8:
			case DataType::Char:
				qlTypeName = "Char";
			case DataType::DateTime:
			case DataType::SmallDateTime:
				qlTypeName = "DateTime";
				break;
			}
		}
		return qlTypeName;
	}
	void IntrospectFields( sv typeName, const DB::Table& dbTable, const DB::TableQL& fieldTable, json& jData )noexcept(false)
	{
		auto fields = json::array();
		var pTypeTable = fieldTable.FindTable( "type" );
		var haveName = fieldTable.ContainsColumn( "name" );
		var pOfTypeTable = pTypeTable->FindTable( "ofType" );
		auto addField = [&]( sv name, sv typeName, DB::QLFieldKind typeKind, sv ofTypeName, optional<DB::QLFieldKind> ofTypeKind )
		{
			json field;
			if( haveName )
				field["name"] = name;
			if( pTypeTable )
			{
				json type;
				auto setField = []( const DB::TableQL& t, json& j, str key, sv x ){ if( t.ContainsColumn(key) ){ if(x.size()) j[key]=x; else j[key]=nullptr; } };
				auto setKind = []( const DB::TableQL& t, json& j, optional<DB::QLFieldKind> pKind )
				{
					if( t.ContainsColumn("kind") )
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
		function<void(const DB::Table&, bool)> addColumns;
		addColumns = [&addColumns,&addField,&typeName]( const DB::Table& dbTable2, bool isMap )
		{
			for( var& column : dbTable2.Columns )
			{
				string fieldName;
				string qlTypeName;
				DB::QLFieldKind rootType = DB::QLFieldKind::Scalar;
				if( column.PKTable.empty() )
				{
					fieldName = DB::Schema::ToJson( column.Name );
					qlTypeName = DB::ColumnQL::QLType( column );//column.PKTable.empty() ? DB::ColumnQL::QLType( column ) : dbTable.JsonTypeName();
				}
				else if( var pPKTable=_schema.Tables.find(column.PKTable); pPKTable!=_schema.Tables.end() )
				{
					if( !isMap || pPKTable->second->IsFlags() ) //!RolePermission || right_id
					{
						rootType = pPKTable->second->IsFlags() ? DB::QLFieldKind::List : DB::QLFieldKind::Object;
						qlTypeName = pPKTable->second->JsonTypeName();
						fieldName = DB::Schema::ToJson( qlTypeName );
						if( pPKTable->second->IsFlags() )
							fieldName = DB::Schema::ToPlural( fieldName );
					}
					else //if( isMap2 )
					{
						if( !typeName.starts_with(pPKTable->second->JsonTypeName()) )//typeName==RolePositions, don't want role columns, just permissions.
							addColumns( *pPKTable->second, false );
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
		addColumns( dbTable, dbTable.IsMap() );
		for( var& [name,pTable] : _schema.Tables )
		{
			auto fnctn = [addField,p=pTable,&dbTable]( var& c1Name, var& c2Name )
			{
				if( var pColumn1=p->FindColumn(c1Name), pColumn2=p->FindColumn(c2Name) ; pColumn1 && pColumn2 /*&& pColumn->PKTable==n*/ )
				{
					if( pColumn1->PKTable==dbTable.Name )
					{
						var jsonType = p->Columns.size()==2 ? _schema.Tables[pColumn2->PKTable]->JsonTypeName() : p->JsonTypeName();
						addField( DB::Schema::ToPlural(DB::Schema::ToJson(jsonType)), {}, DB::QLFieldKind::List, jsonType, DB::QLFieldKind::Object );
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
	void IntrospectEnum( sv /*typeName*/, const DB::Table& dbTable, const DB::TableQL& fieldTable, json& jData )noexcept(false)
	{
		auto fields = json::array();
		ostringstream sql{ "select id,", std::ios::ate };
		vector<string> columns;
		for_each( fieldTable.Columns.begin(), fieldTable.Columns.end(), [&columns, &dbTable](var& x){ if(dbTable.FindColumn(x.JsonName)) columns.push_back(x.JsonName);} );//sb only id/name.
		sql << Str::AddCommas( columns ) << " from " << dbTable.Name << " order by id";
		_pDataSource->Select( sql.str(), [&columns,&fields]( const DB::IRow& row ){ json j; for( uint i=0; i<columns.size(); ++i ) j[columns[i]] = row.GetString(i+1); fields.push_back(j); } );
		json jTable;
		jTable["enumValues"] = fields;
		jData["__type"] = jTable;
	}
	void QueryType( const DB::TableQL& typeTable, json& jData )noexcept(false)
	{
		if( typeTable.Args.find("name")!=typeTable.Args.end() )
		{
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
		else
			THROW( "__type data for all names '{}' not supported" );
	}
	void QuerySchema( const DB::TableQL& schemaTable, json& jData )noexcept(false)
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
			field["name"] = format( "create{}"sv, jsonType );
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
				field["name"] = format( "{}{}", name, jsonType );
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
	α QueryTable( const DB::TableQL& table, uint userId, json& jData )noexcept(false)->void
	{
		TEST_ACCESS( "Read", table.DBName(), userId ); //TODO implement.
		if( table.JsonName=="__type" )
			return QueryType( table, jData );
		else if( table.JsonName=="__schema" )
			return QuerySchema( table, jData );
		var isPlural = table.JsonName.ends_with( "s" );
		var pSyntax = DB::DefaultSyntax();// _pSyntax; THROW_IF( !pSyntax, Exception("SqlSyntax not set.") );
		var pSchemaTable = _schema.FindTableSuffix( table.DBName() ); THROW_IF( !pSchemaTable, "Could not find table '{}' in schema", table.DBName() );
		vector<tuple<string,string>> jsonMembers;
		function<string(const DB::TableQL&, const DB::Table& dbTable, const sp<const DB::Table> pDefTable, vector<uint>&, map<uint,sp<const DB::Table>>&, sv, bool, uint*, string*, vector<tuple<string,string>>* )> columnSql;
		columnSql = [pSyntax, &columnSql]( const DB::TableQL& table2, const DB::Table& dbTable, const sp<const DB::Table> pDefTable, vector<uint>& dates, map<uint,sp<const DB::Table>>& flags, sv defaultPrefix, bool excludeId, uint* pIndex, string* pSubsequentJoin, vector<tuple<string,string>>* pJsonMembers )
		{
			uint index2=0;
			if( !pIndex )
				pIndex = &index2;
			vector<string> columns;
			for( var& column : table2.Columns )
			{
				string columnName = DB::Schema::FromJson( column.JsonName );
				if( columnName=="id" && excludeId )//~~~
					continue;
				auto findColumn = []( const DB::Table& t, str c ){ auto p = t.FindColumn( c ); if( !p ) p = t.FindColumn( c+"_id" ); return p; }; //+_id for enums
				auto pSchemaColumn = findColumn( dbTable, columnName );
				auto prefix = defaultPrefix;
				if( !pSchemaColumn && pDefTable )
				{
					prefix = pDefTable->Name;
					pSchemaColumn = findColumn( *pDefTable, columnName );
					if( !pSchemaColumn )
					{
						pSchemaColumn = findColumn( *pDefTable, DB::Schema::ToSingular(columnName) );
						if( pSchemaColumn )
						{
							if( var p = _schema.Tables.find(pSchemaColumn->PKTable); p!=_schema.Tables.end() )
								flags.emplace( *pIndex, p->second );
							else
								THROW( "Could not find flags pk table for {}.{}", prefix, columnName );
						}
					}
				}
				THROW_IF( !pSchemaColumn, "Could not find column '{}.{}'", dbTable.Name, columnName );//Could not find column 'um_role_permissions.api'" thrown in the test body

				columnName = prefix.size() ? format( "{}.{}", prefix, pSchemaColumn->Name ) : pSchemaColumn->Name;
				bool dateTime = pSchemaColumn->Type==DataType::DateTime;
				if( dateTime )
					dates.push_back( *pIndex );
				columns.push_back( dateTime ? pSyntax->DateTimeSelect(columnName) : columnName );
				if( pJsonMembers )
					pJsonMembers->push_back( make_tuple(table2.JsonName, DB::Schema::ToJson(pSchemaColumn->Name)) );

				++(*pIndex);
			}
			for( var& childTable : table2.Tables )
			{
				var childDbName = childTable->DBName();
				var pPKTable = _schema.FindTableSuffix( childDbName ); THROW_IF( !pPKTable, "Could not find table '{}' in schema", childDbName );
				if( pSubsequentJoin )
				{
					const DB::Column* pColumn = nullptr;
					auto findFK = [pPKTable](var& x){ return x.PKTable==pPKTable->Name; };
					var pDBColumn = find_if( dbTable.Columns.begin(), dbTable.Columns.end(), findFK );
					string childPrefix{ defaultPrefix };
					if( pDBColumn!=dbTable.Columns.end() )
					{
						pColumn = &*pDBColumn;
						childPrefix = dbTable.Name;
					}
					else if( pDefTable )
					{
						childPrefix = pDefTable->Name;
						auto pDefColumn = find_if( pDefTable->Columns.begin(), pDefTable->Columns.end(), findFK );
						if( pDefColumn!=pDefTable->Columns.end() )
							pColumn = &*pDefColumn;
					}
					if( pColumn )
					{
						columns.push_back( columnSql(*childTable, *pPKTable, nullptr, dates, flags, pPKTable->Name, false, pIndex, nullptr, pJsonMembers) );
						*pSubsequentJoin += format( "\tleft join {0} on {0}.id={1}.{2}", pPKTable->Name, childPrefix, pColumn->Name );
					}
				}
			}
			return Str::AddCommas( columns );
		};
		vector<uint> dates; map<uint,sp<const DB::Table>> flags;
		string joins;
		var columnSqlValue = columnSql( table, *pSchemaTable, nullptr, dates, flags, pSchemaTable->Name, false, nullptr, &joins, &jsonMembers );
		ostringstream sql;
		if( columnSqlValue.size() )
			sql << "select " << columnSqlValue;

		var addId = table.Tables.size() && !table.ContainsColumn("id") && table.Columns.size();
		if( addId )
			sql << ", id";
		var& tableName = pSchemaTable->Name;
		if( sql.tellp()!=std::streampos(0) )
		{
			sql << endl << "from\t" << tableName;
			if( joins.size() )
				sql << endl << joins;
		}
		std::vector<DB::DataValue> parameters;
		ostringstream where;
		var pWhere = table.Args.find( "filter" );
		if( pWhere!=table.Args.end() )
		{
			for( var& [name,value] : pWhere->items() )
			{
				var columnName = DB::Schema::FromJson( name );
				var pColumn = pSchemaTable->FindColumn( columnName ); THROW_IF( !pColumn, "column '{}' not found.", columnName );
				if( where.tellp()!=std::streampos(0) )
					where << endl << "\tand ";
				where << tableName << "." << columnName;
				if( !value.is_object() || value.items().begin()==value.items().end() )
					continue;
				var first = value.items().begin();
				if( first.value().is_null() )
					where << " is " << (first.key()=="eq" ? "" : "not ") << "null";
				else
				{
					sv op;
					if( first.key()=="eq" )
						op = "=";
					else if( first.key()=="ne" )
						op = "!=";
					where << op << "?";
					parameters.push_back( ToDataValue(pColumn->Type, first.value(), name) );
				}
			}
		}
		auto rowToJson = [&]( const DB::IRow& row, uint iColumn, json& obj, sv objectName, sv memberName, const vector<uint>& dates, const flat_map<uint,sp<flat_map<uint,string>>>& flagValues )
		{
			DB::DataValue value = row[iColumn];
			var index = (EDataValue)value.index();
			if( index==EDataValue::Null )
				return;
			auto& member = objectName.empty() ? obj[string{memberName}] : obj[string{objectName}][string{memberName}];
			if( index==EDataValue::String )
				member = get<string>( value );
			else if( index==EDataValue::StringView )
				member = get<sv>( value );
			else if( index==EDataValue::Bool )
				member = get<bool>( value );
			else if( index==EDataValue::Int64 )
			{
				var intValue = get<_int>( value );
				if( find(dates.begin(), dates.end(), iColumn)!=dates.end() )
					member = DateTime( intValue ).ToIsoString();
				else
					member = get<_int>( value );
			}
			else if( index==EDataValue::Uint || index==EDataValue::Int )
			{
				if( var pFlagValues = flagValues.find(iColumn); pFlagValues!=flagValues.end() )
				{
					member = json::array();
					uint remainingFlags = index==EDataValue::Uint ? get<uint>( value ) : get<int>( value );
					for( uint iFlag=0x1; remainingFlags!=0; iFlag <<= 1 )
					{
						if( (remainingFlags & iFlag)==0 )
							continue;
						if( var pValue = pFlagValues->second->find(iFlag); pValue!=pFlagValues->second->end() )
							member.push_back( pValue->second );
						else
							member.push_back( std::to_string(iFlag) );
						remainingFlags -= iFlag;
					}
				}
				else if( index==EDataValue::Uint )
					member = get<uint>( value );
				else
					member = get<int>( value );
			}
			else if( index==EDataValue::Decimal2 )
				member = (float)get<Decimal2>( value );
			else if( index==EDataValue::Double )
				member = get<double>( value );
			else if( index==EDataValue::DoubleOptional )
				member = get<optional<double>>(value).value();
			else if( index==EDataValue::DateOptional )
				member = ToIsoString( get<optional<DB::DBDateTime>>(value).value() );
			else if( index==EDataValue::StringPtr )
			{
				if( get<sp<string>>(value) )
					member = *get<sp<string>>(value);
			}
			else
				ERR( "{} not implemented"sv, (uint8)index );
		};
		flat_map<string,flat_multimap<uint,json>> subTables;
		auto selectSubTables = [&subTables, &columnSql, &rowToJson, &parameters]( const vector<sp<const DB::TableQL>>& tables, const DB::Table& parentTable, sv where2 )
		{
			for( var& pQLTable : tables )
			{
				auto pSubTable = _schema.FindTableSuffix( pQLTable->DBName() ); THROW_IF( !pSubTable, "Could not find table '{}' in schema", pQLTable->DBName() );
				sp<const DB::Table> pDefTable;
				if( pSubTable->IsMap() )//for RolePermissions, subTable=Permissions, defTable=RolePermissions
				{
					pDefTable = pSubTable;
					var subIsParent = pDefTable->ChildId()==parentTable.FKName();
					pSubTable = subIsParent ? pDefTable->ParentTable( _schema ) : pDefTable->ChildTable( _schema ); THROW_IF( !pSubTable, "Could not find {} table for '{}' in schema", (subIsParent ? "parent" : "child"), pDefTable->Name );
				}
				else
					pDefTable = _schema.FindDefTable( parentTable, *pSubTable );
				if( !pDefTable )
					continue;  //'um_permissions<->'apis' //THROW_IF( !pDefTable, Exception("Could not find def table '{}<->'{}' in schema", parentTable.Name, pQLTable->DBName()) );

				var defTableName = pDefTable->Name;
				ostringstream subSql{ format("select {0}.{1} primary_id, {0}.{2} sub_id", defTableName, parentTable.FKName(), pSubTable->FKName()), std::ios::ate };
				var& subTableName = pSubTable->Name;
				vector<uint> subDates; map<uint,sp<const DB::Table>> subFlags;
				string subsequentJoin;
				var columns = columnSql( *pQLTable, *pSubTable, pDefTable, subDates, subFlags, subTableName, true, nullptr, &subsequentJoin, nullptr );
				if( columns.size() )
					subSql << ", " << columns;
				subSql << "\nfrom\t" << parentTable.Name << endl
					<< "\tjoin " << defTableName << " on " << parentTable.Name <<".id=" << defTableName << "." << parentTable.FKName();
				if( columns.size() )
				{
					subSql << "\tjoin " << subTableName << " on " << subTableName <<".id=" << defTableName << "." << pSubTable->FKName();
					if( subsequentJoin.size() )
						subSql << endl << subsequentJoin;
				}

				if( where2.size() )
					subSql << endl << "where\t" << where2;
				flat_map<uint,sp<flat_map<uint,string>>> subFlagValues;
				for( var& [index,pTable] : subFlags )
					subFlagValues[index+2] = _pDataSource->SelectMap<uint,string>( format("select id, name from {}"sv, pTable->Name) );
				auto& rows = subTables.emplace( pQLTable->JsonName, flat_multimap<uint,json>{} ).first->second;
				auto forEachRow = [&]( const DB::IRow& row )
				{
					json jSubRow;
					uint index = 0;
					var rowToJson2 = [&row, &subDates, &rowToJson, &subFlagValues]( const vector<DB::ColumnQL>& columns, bool checkId, json& jRow, uint& index2 )
					{
						for( var& column : columns )
						{
							auto i = checkId && column.JsonName=="id" ? 1 : (index2++)+2;
							rowToJson( row, i, jRow, "", column.JsonName, subDates, subFlagValues );
						}
					};
					rowToJson2( pQLTable->Columns, true, jSubRow, index );
					for( var& childTable : pQLTable->Tables )
					{
						var childDbName = childTable->DBName();
						var pPKTable = _schema.FindTableSuffix( childDbName ); THROW_IF( !pPKTable, "Could not find table '{}' in schema", childDbName );
						var pColumn = find_if( pSubTable->Columns.begin(), pSubTable->Columns.end(), [pPKTable](var& x){ return x.PKTable==pPKTable->Name; } );
						if( pColumn!=pSubTable->Columns.end() )
						{
							json jChildTable;
							rowToJson2( childTable->Columns, false, jChildTable, index );
							jSubRow[childTable->JsonName] = jChildTable;
						}
					}
					rows.emplace( row.GetUInt(0), jSubRow );
				};
				_pDataSource->Select( subSql.str(), forEachRow, parameters );
			}
		};
		selectSubTables( table.Tables, *pSchemaTable, where.str() );
		var jsonTableName = table.JsonName;
		auto addSubTables = [&]( json& jParent, uint id=0 )
		{
			for( var& pQLTable : table.Tables )
			{
				var subPlural = pQLTable->JsonName.ends_with( "s" );
				if( subPlural )
					jParent[pQLTable->JsonName] = json::array();
				var pResultTable = subTables.find( pQLTable->JsonName );
				if( pResultTable==subTables.end() )
					continue;
				var& subResults = pResultTable->second;
				if( !id && subResults.size() )
					id = subResults.begin()->first;
				auto range = subResults.equal_range( id );
				for( auto pRow = range.first; pRow!=range.second; ++pRow )
				{
					if( subPlural )
						jParent[pQLTable->JsonName].push_back( pRow->second );
					else
						jParent[pQLTable->JsonName] = pRow->second;
				}
			}
		};
		if( sql.tellp()!=std::streampos(0) )
		{
			if( where.tellp()!=std::streampos(0) )
				sql << endl << "where\t" << where.str();
			if( isPlural )
				jData[jsonTableName] = json::array();
			auto primaryRow = [&]( const DB::IRow& row )
			{
				json jRow;
				for( uint i=0; i<jsonMembers.size(); ++i )
				{
					var parent = get<0>(jsonMembers[i]);
					rowToJson( row, i, jRow, parent==jsonTableName ? sv{} : parent, get<1>(jsonMembers[i]), dates, {} );
				}
				if( subTables.size() )
					addSubTables( jRow );
				if( isPlural )
					jData[jsonTableName].push_back( jRow );
				else
					jData[jsonTableName] = jRow;
			};
			_pDataSource->Select( sql.str(), primaryRow, parameters );
		}
		else
		{
			json jRow;
			addSubTables( jRow );
			jData[jsonTableName] = jRow;
		}
	}

	α QueryTables( const vector<DB::TableQL>& tables, uint userId )noexcept(false)->json
	{
		json data;
		for( var& table : tables )
			QueryTable( table, userId, data["data"] );
		return data;
	}


	json DB::Query( sv query, UserPK userId )noexcept(false)
	{
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

		return tableQueries.size() ? QueryTables( tableQueries, userId ) : j;
	}
	struct Parser
	{
		Parser( sv text, sv delimiters )noexcept: _text{text}, Delimiters{delimiters}{}
		sv Next()noexcept
		{
			sv result = _peekValue;
			if( result.empty() )
			{
				if( i<_text.size() )
					for( auto ch = _text[i]; i<_text.size() && isspace(ch); ch = i<_text.size()-1 ? _text[++i] : _text[i++] );
				if( i<_text.size() )
				{
					uint start=i;
					i = start+std::distance( _text.begin()+i, find_if(_text.begin()+i, _text.end(), [this]( char ch )noexcept{ return isspace(ch) || Delimiters.find(ch)!=sv::npos;}) );
					result = i==start ? _text.substr( i++, 1 ) : _text.substr( start, i-start );
				}
			}
			else
				_peekValue = {};
			return result;
		};
		sv Next( char end )noexcept
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
		sv Peek()noexcept{ return _peekValue.empty() ? _peekValue = Next() : _peekValue; }
		uint Index()noexcept{ return i;}
		sv Text()noexcept{ return i<_text.size() ? _text.substr(i) : sv{}; }
		sv AllText()noexcept{ return _text; }
	private:
		uint i{0};
		sv _text;
		sv Delimiters;
		sv _peekValue;
	};
	string StringifyKeys( sv json )
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
	json ParseJson( Parser& q )
	{
		string params{ q.Next(')') }; THROW_IF( params.front()!='(', "Expected '(' vs {} @ '{}' to start function - '{}'.",  params.front(), q.Index()-1, q.Text() );
		params.front()='{'; params.back() = '}';
		params = StringifyKeys( params );
		return json::parse( params );
	}
	DB::TableQL LoadTable( Parser& q, sv jsonName )noexcept(false)//__type(name: "Account") { fields { name type { name kind ofType{name kind} } } }
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
	vector<DB::TableQL> LoadTables( Parser& q, sv jsonName )noexcept(false)
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

	DB::MutationQL LoadMutation( Parser& q )
	{
		THROW_IF( q.Next()!="{", "mutation expected '{' as 1st character." );
		var command = q.Next();
		uint iType=0;
		for( ;iType<DB::MutationQLStrings.size() && !command.starts_with(DB::MutationQLStrings[iType]); ++iType ); THROW_IF( iType==DB::MutationQLStrings.size(), "Could not find mutation {}", command );

		auto tableJsonName = string{ command.substr(DB::MutationQLStrings[iType].size()) };
		tableJsonName[0] = (char)tolower( tableJsonName[0] );
		var type = (DB::EMutationQL)iType;
		var j = ParseJson( q );
		var wantsResult  = q.Peek()=="{";
		optional<DB::TableQL> result = wantsResult ? LoadTable( q, tableJsonName ) : optional<DB::TableQL>{};
		DB::MutationQL ql{ string{tableJsonName}, type, j, result/*, parentJsonName*/ };
		return ql;
	}
	DB::RequestQL DB::ParseQL( sv query )noexcept(false)
	{
		uint i = query.find_first_of( "{" );
		THROW_IF( i==sv::npos || i>query.size()-2, "Invalid query '{}'", query );
		Parser parser{ query.substr(i+1), "{}()," };
		auto name = parser.Next();
		if( name=="query" )
			name = parser.Next();

		return name=="mutation" ? DB::RequestQL{ LoadMutation(parser) } : DB::RequestQL{ LoadTables(parser,name) };
	}
}