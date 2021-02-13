#include "GraphQL.h"
#include "SchemaProc.h"
#include "Row.h"
#include "DataSource.h"
#include "SqlSyntax.h"
#include "../DateTime.h"
#include "../StringUtilities.h"
#include "../TypeDefs.h"

#define var const auto
namespace Jde
{
	using nlohmann::json;
	using DB::EDataValue;
	using DB::DataType;
	sp<DB::IDataSource> _pDataSource;
	sp<DB::SqlSyntax> _pSyntax;
	void DB::SetQLDataSource( sp<DB::IDataSource> p, sp<DB::SqlSyntax> pSyntax )noexcept{ _pDataSource = p; _pSyntax = pSyntax; }
	void DB::ClearQLDataSource()noexcept{ _pDataSource = nullptr; _pSyntax = nullptr; }
	DB::Schema _schema;
	void DB::AppendQLSchema( const DB::Schema& schema )noexcept
	{
		for( var& [name,v] : schema.Types )
			_schema.Types.emplace( name, v );
		for( var& [name,v] : schema.Tables )
		{
// 			if( name=="um_users" )
// 			{
// 				var pColumn = v->FindColumn( "name" );
// //				DBG( "{}.{}, dt={}"sv, name, pColumn->Name, pColumn->DataTypeString() );
// 			}
			_schema.Tables.emplace( name, v );
		}
	}
	nlohmann::json InputParam2( sv name, const nlohmann::json& input )noexcept(false)
	{
		var p = input.find( name ); THROW_IF( p==input.end(), Exception("Could not find '{}' argument. {}", name, input.dump()) );
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
		var pInput = Args.find("input"); THROW_IF( pInput==Args.end(), Exception("Could not find input argument. {}", Args.dump()) );
		return *pInput;
	}

	PK MutationQL::ParentPK()const noexcept(false)
	{
		return InputParam( "parentId" ).get<PK>();
	}
	PK MutationQL::ChildPK()const noexcept(false)
	{
		return InputParam( "childId" ).get<PK>();
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
		var pInput = m.Args.find( "input" ); THROW_IF( pInput == m.Args.end(), Exception("Could not find 'input' arg.") );
		for( var& column : table.Columns )
		{
			if( !column.Insertable )
				continue;
			var memberName = DB::Schema::ToJson( column.Name );
			var pValue = pInput->find( memberName );

			var exists = pValue!=pInput->end(); THROW_IF( !exists && !column.IsNullable && column.Default.empty() && column.Insertable, Exception("No value specified for {}.", memberName) );
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
	uint Update( const DB::Table& table, const DB::MutationQL& m, const DB::SqlSyntax& syntax )
	{
		ostringstream sql{ "update ", std::ios::ate }; sql << table.Name << " set ";
		std::vector<DB::DataValue> parameters; parameters.reserve( table.Columns.size() );
		var pId = m.Args.find( "id" ); THROW_IF( pId==m.Args.end(), Exception("Could not find id argument. {}", m.Args.dump()) );
		var pInput = m.Args.find("input"); THROW_IF( pInput==m.Args.end(), Exception("Could not find input argument. {}", m.Args.dump()) );
		string sqlUpdate;
		for( var& column : table.Columns )
		{
			if( column.IsIdentity )
				continue;
			if( !column.Updateable )
			{
				if( column.Name=="updated" )
					sqlUpdate = format( ",{}={}", column.Name, syntax.UtcNow() );//TODO use sqlsyntax
				continue;
			}
			var memberName = DB::Schema::ToJson( column.Name );
			var pValue = pInput->find( memberName );
			if( pValue==pInput->end() )
				continue;
			if( parameters.size() )
				sql << ", ";
			sql << column.Name << "=?";
			parameters.push_back( ToDataValue(column.Type, *pValue, memberName) );
		}
		THROW_IF( parameters.size()==0, Exception("There is nothing to update.") );
		if( sqlUpdate.size() )
			sql << sqlUpdate;
		sql << " where id=?";
		parameters.push_back( ToDataValue(DataType::ULong, *pId, "id") );
		return _pDataSource->Execute( sql.str(), parameters );
	}
	uint Delete( const DB::Table& table, const DB::MutationQL& m, const DB::SqlSyntax& syntax )
	{
		var pId = m.Args.find( "id" ); THROW_IF( pId==m.Args.end(), Exception("Could not find id argument. {}", m.Args.dump()) );
		var pColumn = find_if( table.Columns.begin(), table.Columns.end(), []( var& c ){ return c.Name=="deleted"; } ); THROW_IF( pColumn==table.Columns.end(), Exception("Could not find 'deleted' column") );
		std::vector<DB::DataValue> parameters;
		parameters.push_back( ToDataValue(DataType::ULong, *pId, "id") );
		ostringstream sql{ "update ", std::ios::ate }; sql << table.Name << " set deleted=" << syntax.UtcNow() << " where id=?";
		return _pDataSource->Execute( sql.str(), parameters );
	}
	uint Restore( const DB::Table& table, const DB::MutationQL& m )
	{
		var pId = m.Args.find( "id" ); THROW_IF( pId==m.Args.end(), Exception("Could not find id argument. {}", m.Args.dump()) );
		var pColumn = find_if( table.Columns.begin(), table.Columns.end(), []( var& c ){ return c.Name=="deleted"; } ); THROW_IF( pColumn==table.Columns.end(), Exception("Could not find 'deleted' column") );
		std::vector<DB::DataValue> parameters;
		parameters.push_back( ToDataValue(DataType::ULong, *pId, "id") );
		ostringstream sql{ "update ", std::ios::ate }; sql << table.Name << " set deleted=null where id=?";
		return _pDataSource->Execute( sql.str(), parameters );
	}
	uint Purge( sv tableName, const DB::MutationQL& m, const DB::SqlSyntax& syntax )
	{
		var pId = m.Args.find( "id" ); THROW_IF( pId==m.Args.end(), Exception("Could not find id argument. {}", m.Args.dump()) );
		std::vector<DB::DataValue> parameters;
		parameters.push_back( ToDataValue(DataType::ULong, *pId, "id") );
		ostringstream sql{ "delete from ", std::ios::ate }; sql << tableName << " where id=?";
		return _pDataSource->Execute( sql.str(), parameters );
	}

	std::vector<DB::DataValue> ChildParentParams( const json& input )
	{
		std::vector<DB::DataValue> parameters;
		parameters.push_back( ToDataValue(DataType::ULong, InputParam2("childId", input), "childId") );
		parameters.push_back( ToDataValue(DataType::ULong, InputParam2("parentId", input), "parentId") );
		return parameters;
	};
	uint Add( const DB::Table& table, const json& input, const DB::SqlSyntax& syntax )
	{
		ostringstream sql{ "insert into ", std::ios::ate }; sql << table.Name << "(" << table.ChildId() << "," << table.ParentId();
		auto parameters = ChildParentParams(input);
		ostringstream values{ "?,?", std::ios::ate };
		for( var& [name,value] : input.items() )
		{
			if( name=="childId" || name=="parentId" )
				continue;
			var columnName = DB::Schema::FromJson( name );
			auto pColumn = table.FindColumn( columnName ); THROW_IF(!pColumn, Exception("Could not find column {}.{}.", table.Name, columnName) );
			sql << "," << columnName;
			values << ",?";

			parameters.push_back( ToDataValue(pColumn->Type, value, name) );
		}
		sql << ")values( " << values.str() << ")";
		return _pDataSource->Execute( sql.str(), parameters );
	}
	uint Remove( const DB::Table& table, const json& input, const DB::SqlSyntax& syntax )
	{
		ostringstream sql{ "delete from ", std::ios::ate }; sql << table.Name << " where " << table.ChildId() << "=? and " << table.ParentId() << "=?";
		return _pDataSource->Execute( sql.str(), ChildParentParams(input) );
	}

	uint Mutation( const DB::MutationQL& m, UserPK userId )noexcept(false)
	{
		var pSyntax = _pSyntax; THROW_IF( !pSyntax, Exception("SqlSyntax not set.") );
		var pSchemaTable = _schema.FindTableSuffix( m.TableSuffix() ); THROW_IF( !pSchemaTable, Exception("Could not find table suffixed with '{}' in schema", m.TableSuffix()) );
		TEST_ACCESS( "Write", pSchemaTable->Name, userId ); //TODO implement.
		uint result;
		if( m.Type==DB::EMutationQL::Create )
			result = Insert( *pSchemaTable, m );
		else if( m.Type==DB::EMutationQL::Update )
			result = Update( *pSchemaTable, m, *pSyntax );
		else if( m.Type==DB::EMutationQL::Delete )
			result = Delete( *pSchemaTable, m, *pSyntax );
		else if( m.Type==DB::EMutationQL::Restore )
			result = Restore( *pSchemaTable, m );
		else if( m.Type==DB::EMutationQL::Purge )
			result = Purge( pSchemaTable->Name, m, *pSyntax );
		else
		{
			if( m.Type==DB::EMutationQL::Add )
				result = Add( *pSchemaTable, m.Input(), *pSyntax );
			else if( m.Type==DB::EMutationQL::Remove )
				result = Remove( *pSchemaTable, m.Input(), *pSyntax );
			else
				throw "unknown type";
		}
		if( pSchemaTable->Name.starts_with("um_") )
			UM::ApplyMutation( m, result );

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
				THROW( Exception("DataType {} is not implemented.", column.Type) );
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
	void IntrospectFields( const DB::Table& dbTable, const DB::TableQL& fieldTable, json& jData )noexcept(false)
	{
		auto fields = json::array();
		var pTypeTable = fieldTable.FindTable( "type" );
		var haveName = fieldTable.ContainsColumn( "name" );
		var pOfTypeTable = pTypeTable->FindTable( "ofType" );
		//var ofTypeColumns = pOfTypeTable ? pOfTypeTable->Columns : vector<DB::ColumnQL>{};
		auto addField = [&]( sv name, sv typeName, DB::QLFieldKind typeKind, sv ofTypeName, optional<DB::QLFieldKind> ofTypeKind )
		{
			json field;
			if( haveName )
				field["name"] = name;
			if( pTypeTable )
			{
				json type;
				auto setField = []( const DB::TableQL& t, json& j, const string& key, sv x ){ if( t.ContainsColumn(key) ){ if(x.size()) j[key]=x; else j[key]=nullptr; } };
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
					// if( find_if(ofTypeColumns.begin(),ofTypeColumns.end(), [&](var& x){return x.JsonName=="name";})!=ofTypeColumns.end() )
					// if( find_if(ofTypeColumns.begin(),ofTypeColumns.end(), [&](var& x){return x.JsonName=="kind";})!=ofTypeColumns.end() )
					setField( *pOfTypeTable, ofType, "name", ofTypeName );
					setKind( *pOfTypeTable, ofType, ofTypeKind );
					type["ofType"] = ofType;
				}
				field["type"] = type;
			}
			fields.push_back( field );
		};
		for( var& column : dbTable.Columns )
		{
			var fieldName = DB::Schema::ToJson( column.Name );
			// json field;
			// if( haveName )
			// 	field["name"] = fieldName;
			// if( pTypeTable )
			// {
			// 	json type;
			// 	if( pTypeTable->ContainsColumn("name") )
			// 		type["name"] = nullptr;
			// 	if( pTypeTable->ContainsColumn("kind") && !column.IsNullable )
			// 		type["kind"] = "NON_NULL";
			// 	json ofType;
			// 	auto& scalarField = column.IsNullable ? type : ofType;
			// 	var& scalarColumns = column.IsNullable ? pTypeTable->Columns : ofTypeColumns;
			// 	if( find_if(scalarColumns.begin(),scalarColumns.end(), [&](var& x){return x.JsonName=="kind";})!=scalarColumns.end() )
			// 		scalarField["kind"] = "SCALAR";
			var qlTypeName = DB::ColumnQL::QLType( column );
//				if( find_if( scalarColumns.begin(), scalarColumns.end(), [](var& c){return c.JsonName=="name";})!=scalarColumns.end() )
//					scalarField["name"] = qlTypeName;
//				if( pOfTypeTable )
//					type["ofType"] = ofType;
//				field["type"] = type;
			var isNullable = column.IsNullable;
			string typeName = isNullable ? qlTypeName : "";
			var typeKind = isNullable ? DB::QLFieldKind::Scalar : DB::QLFieldKind::NonNull;
			string ofTypeName = isNullable ? "" : qlTypeName;
			var ofTypeKind = isNullable ? optional<DB::QLFieldKind>{} : DB::QLFieldKind::Scalar;

			addField( fieldName, typeName, typeKind, ofTypeName, ofTypeKind );
		}
		for( var& [name,pTable] : _schema.Tables )
		{
			auto fnctn = [addField,p=pTable,&dbTable]( var& c1Name, var& c2Name )
			{
				if( var pColumn1=p->FindColumn(c1Name), pColumn2=p->FindColumn(c2Name) ; pColumn1 && pColumn2 /*&& pColumn->PKTable==n*/ )
				{
					if( pColumn1->PKTable==dbTable.Name )
					{
						var jsonType = _schema.Tables[pColumn2->PKTable]->JsonTypeName();
						addField( DB::Schema::ToPlural(DB::Schema::ToJson(jsonType)), {}, DB::QLFieldKind::List, jsonType, DB::QLFieldKind::Object );
					}
				}
			};
			var child = pTable->ChildId();
			var parent = pTable->ParentId();
			fnctn( child, parent );
			fnctn( parent, child );
		}
		json jTable;
		jTable["name"] = dbTable.JsonTypeName();
		jTable["fields"] = fields;
		jData["__type"] = jTable;
	}
	void QueryType( const DB::TableQL& typeTable, json& jData )noexcept(false)
	{
		//DBG( typeTable.Args.dump() );
		if( typeTable.Args.find("name")!=typeTable.Args.end() )
		{
			var typeName = typeTable.Args["name"].get<string>();
			//var dbName = DB::Schema::ToPlural( DB: :Schema::FromJson(typeName) );
			auto p = std::find_if( _schema.Tables.begin(), _schema.Tables.end(), [&](var& t){ return t.second->JsonTypeName()==typeName;} );
			THROW_IF( p==_schema.Tables.end(), Exception("Could not find table '{}' in schema", typeName) );
			for( var& pQlTable : typeTable.Tables )
			{
				if( pQlTable->JsonName=="fields" )
					IntrospectFields( *p->second, *pQlTable, jData );
				else
					THROW( Exception("__type data for '{}' not supported", pQlTable->JsonName) );
			}
		}
		else
			THROW( Exception("__type data for all names '{}' not supported") );
	}
	void QuerySchema( const DB::TableQL& schemaTable, json& jData )noexcept(false)
	{
		THROW_IF( schemaTable.Tables.size()!=1, Exception("Only Expected 1 table type for __schema {}", schemaTable.Tables.size()) );
		var pMutationTable = schemaTable.Tables[0]; THROW_IF( pMutationTable->JsonName!="mutationType", Exception("Only mutationType implemented for __schema - {}", pMutationTable->JsonName) );
		auto fields = json::array();
		for( var& nameTablePtr : _schema.Tables )
		{
			var pDBTable = nameTablePtr.second;
			var childId = pDBTable->ChildId();
			var jsonType = pDBTable->JsonTypeName();

			json field;
			field["name"] = format( "create{}", jsonType );
			auto args = json::array();
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
	void QueryTable( const DB::TableQL& table, uint userId, json& jData )noexcept(false)
	{
		TEST_ACCESS( "Read", table.DBName(), userId ); //TODO implement.
		if( table.JsonName=="__type" )
			return QueryType( table, jData );
		else if( table.JsonName=="__schema" )
			return QuerySchema( table, jData );
		var isPlural = table.JsonName.ends_with( "s" );
		var pSyntax = _pSyntax; THROW_IF( !pSyntax, Exception("SqlSyntax not set.") );
		ostringstream sql{ "select ", std::ios::ate };
		vector<string> jsonMembers;
		var pSchemaTable = _schema.FindTableSuffix( table.DBName() ); THROW_IF( !pSchemaTable, Exception("Could not find table '{}' in schema", table.DBName()) );
		for( var& column : table.Columns )
		{
			jsonMembers.emplace_back( column.JsonName );
			string columnName = DB::Schema::FromJson( column.JsonName );
			var pSchemaColumn = pSchemaTable->FindColumn( columnName ); THROW_IF( !pSchemaColumn, Exception("Could not find column '{}'", columnName) );
		}
		auto columnSql = [pSyntax]( const DB::TableQL& table2, vector<uint>& dates, sv prefix={}, bool excludeId=false )
		{
			ostringstream os;
			uint index = 0;
			var pDBTable = _schema.FindTableSuffix( table2.DBName() ); THROW_IF( !pDBTable, Exception("Could not find table '{}' in schema", table2.DBName()) );
			for( var& column : table2.Columns )
			{
				string columnName = DB::Schema::FromJson( column.JsonName );
				if( columnName=="id" && excludeId )
					continue;
				var pSchemaColumn = pDBTable->FindColumn( columnName ); THROW_IF( !pSchemaColumn, Exception("Could not find column '{}.{}'", pDBTable->Name, columnName) );
				os << ( index==0 ? "" : ", " );
				if( prefix.size() )
					columnName = format( "{}.{}", prefix, columnName );
				if( pSchemaColumn->Type==DataType::DateTime )
				{
					dates.push_back( index );
					os << pSyntax->DateTimeSelect( columnName );
				}
				else
					os << columnName;

				++index;
			}
			return os.str();
		};
		vector<uint> dates;
		sql << columnSql( table, dates );

		var addId = table.Tables.size() && !table.ContainsColumn("id");
		if( addId )
			sql << ", id";
		var& tableName = pSchemaTable->Name;
		sql << endl << "from\t" << tableName;
		std::vector<DB::DataValue> parameters;
		ostringstream where;
		if( !table.Args.empty() )
		{
			for( var& [name,value] : table.Args.items() )
			{
				var columnName = DB::Schema::FromJson( name );
				var pColumn = pSchemaTable->FindColumn( columnName ); THROW_IF( !pColumn, Exception("column '{}' not found.", columnName) );
				if( where.tellp()!=std::streampos(0) )
					where << endl << "\tand ";
				where << tableName << "." << columnName;
				if( value.is_null() )
					where << " is null";
				else
				{
					where << "=?";
					parameters.push_back( ToDataValue(pColumn->Type, value, name) );
				}
			}
			sql << endl << "where\t" << where.str();
		}
		auto rowToJson = [&]( const DB::IRow& row, uint iColumn, json& obj, sv memberName, const vector<uint>& dates )
		{
			DB::DataValue value = row[iColumn];
			var index = (EDataValue)value.index();
			if( index==EDataValue::Null )
				return;
			auto& member = obj[string{memberName}];
			if( index==EDataValue::String )
				member = get<string>( value );
			else if( index==EDataValue::StringView )
				member = get<string_view>( value );
			else if( index==EDataValue::Bool )
				member = get<bool>( value );
			else if( index==EDataValue::Int )
				member = get<int>( value );
			else if( index==EDataValue::Int64 )
			{
				var intValue = get<_int>( value );
				if( find(dates.begin(), dates.end(), iColumn)!=dates.end() )
					member = DateTime( intValue ).ToIsoString();
				else
					member = get<_int>( value );
			}
			else if( index==EDataValue::Uint )
				member = get<uint>( value );
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
		for( var& pQLTable : table.Tables )
		{
			var pSubTable = _schema.FindTableSuffix( pQLTable->DBName() ); THROW_IF( !pSubTable, Exception("Could not find table '{}' in schema", pQLTable->DBName()) );
			var pDefTable = _schema.FindDefTable( *pSchemaTable, *pSubTable ); THROW_IF( !pDefTable, Exception("Could not find def table '{}<->'{}' in schema", pSchemaTable->Name, pQLTable->DBName()) );
			var defTableName = pDefTable->Name;
			//var& subName = subTable.subName;
			ostringstream subSql{ format("select {0}.{1} primary_id, {0}.{2} sub_id", defTableName, pSchemaTable->FKName(), pSubTable->FKName()), std::ios::ate };
			var& subTableName = pSubTable->Name;
			vector<uint> subDates;
			var columns = columnSql( *pQLTable, subDates, subTableName, true );
			if( columns.size() )
				subSql << ", " << columns;
			subSql << "\nfrom\t" << tableName
				<< "\tjoin " << defTableName << " on " << tableName <<".id=" << defTableName << "." << pSchemaTable->FKName();
			if( columns.size() )
				subSql << "\tjoin " << subTableName << " on " << subTableName <<".id=" << defTableName << "." << pSubTable->FKName();

			if( where.tellp()!=std::streampos(0) )
				subSql << endl << "where\t" << where.str();
			auto& rows = subTables.emplace( pQLTable->JsonName, flat_multimap<uint,json>{} ).first->second;
			auto forEachRow = [&]( const DB::IRow& row )
			{
				json jSubRow;
				uint index = 0;
				for( var& column : pQLTable->Columns )
				{
					auto i = column.JsonName=="id" ? 1 : (index++)+2;
					rowToJson( row, i, jSubRow, column.JsonName, subDates );
				}
				rows.emplace( get<uint>(row[1]), jSubRow );
			};
			_pDataSource->Select( subSql.str(), forEachRow, parameters );
		}

		var jsonTableName = table.JsonName;// DB::Schema::ToJson( table.DBName() )
		auto fnctn = [&]( const DB::IRow& row )
		{
			json j;
			uint id;
			for( uint i=0; i<jsonMembers.size(); ++i )
			{
				rowToJson( row, i, j, jsonMembers[i], dates );
				if( jsonMembers[i]=="id" )
					id = j[jsonMembers[i]].get<uint>();
			}
			if( subTables.size() )
			{
				if( addId )
					id = get<uint>( row[jsonMembers.size()] );
				for( var& pQLTable : table.Tables )
				{
					//var pSubTable = _schema.FindTableSuffix( subTable.DBName() );
					var subPlural = pQLTable->JsonName.ends_with( "s" );
					if( subPlural )
						j[pQLTable->JsonName] = json::array();
					var& subResults = subTables.find( pQLTable->JsonName )->second;
					auto range = subResults.equal_range( id );
					for( auto pRow = range.first; pRow!=range.second; ++pRow )
					{
						if( subPlural )
							j[pQLTable->JsonName].push_back( pRow->second );
						else
							j[pQLTable->JsonName] = pRow->second;
					}
				}
			}
			if( isPlural )
				jData[jsonTableName].push_back( j );
			else
				jData[jsonTableName] = j;
		};
		if( isPlural )
			jData[jsonTableName] = json::array();

		DBG( sql.str() );
		_pDataSource->Select( sql.str(), fnctn, parameters );
	}

	json QueryTables( const vector<DB::TableQL>& tables, uint userId )noexcept(false)
	{
		json data;
		for( var& table : tables )
			QueryTable( table, userId, data["data"] );
		return data;
	}


	json DB::Query( sv query, UserPK userId )noexcept(false)
	{
		DBG( query );
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
		Parser( sv text, const vector<char>& delimiters )noexcept: _text{text}, Delimiters{delimiters}{}
		sv Next()noexcept
		{
			sv result = _peekValue;
			if( result.empty() )
			{
				for( auto ch = _text[i]; i<_text.size() && isspace(ch); ch = _text[++i] );
				if( i<_text.size() )
				{
					uint start=i;
					for( auto ch = _text[i]; i<_text.size() && !isspace(ch) && find(Delimiters.begin(), Delimiters.end(), ch)==Delimiters.end(); ch = _text[++i] );
					if( i==start )
						++i;
					result = _text.substr( start, i-start );
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
	private:
		uint i{0};
		sv _text;
		vector<char> Delimiters;
		sv _peekValue;
	};
	string StringifyKeys( sv json )
	{
		ostringstream os;
		bool inValue = false;
		uint i=0;
		for( char ch = json[i]; i<json.size(); ch = json[++i] )
		{
			if( ch=='{' || ch=='}' || std::isspace(ch) )
				os << ch;
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
				for( char ch = '"'; i<json.size() && ch!=':'; ch=json[++i] )
					os << ch;
				if( !quoted )
					os << '"';
				os << ":";
				inValue = true;
			}
		}
		DBG( os.str() );
		return os.str();
		// auto paramStringA = string{ q.Next(')') }; THROW_IF( paramStringA.front()!='(', Exception("Expected '(' vs {} @ '{}' to start function - '{}'.",  paramStringA.front(), q.Index()-1, q.Text()) );
		// var lastIndex = paramStringA.find_last_of( ')' ); THROW_IF( lastIndex==string::npos, Exception("Expected ')' - '{}'.",  paramStringA, q.Text()) );
		// var paramString = paramStringA.substr( 1, lastIndex-1 );
		// var params = StringUtilities::Split( paramString );
		// json j;
		// for( var& param : params )
		// {
		// 	auto keyValue = StringUtilities::Split( param, ':' ); THROW_IF( keyValue.size()!=2, Exception("Could not parse {} keyValue.size()!=2", paramString) );
		// 	auto key = keyValue[0]; StringUtilities::Trim(key);
		// 	auto value  = keyValue[1]; StringUtilities::Trim(value); THROW_IF( key.empty() || value.empty(), Exception("Could not parse {} key.empty() || value.empty()", paramString) );
		// 	if( value.starts_with("\"") && value.ends_with("\"") )
		// 	{
		// 		THROW_IF( value.size()==1, Exception("Could not parse {}.  value.size()==1", paramString) );
		// 		j[key] = value.size()==2 ? "" : value.substr( 1, value.size()-2 );
		// 	}
		// 	else if( value=="null" )
		// 		j[key] = nullptr;
		// 	else if( value=="true" )
		// 		j[key] = true;
		// 	else if( value=="false" )
		// 		j[key] = false;
		// 	else
		// 	{
		// 		istringstream is{ value };
		// 		double v2; is >> v2;
		// 		j[key] = v2;
		// 	}
		// }
		// return j;
	}
	json ParseJson( Parser& q )
	{
		string params{ q.Next(')') }; THROW_IF( params.front()!='(', Exception("Expected '(' vs {} @ '{}' to start function - '{}'.",  params.front(), q.Index()-1, q.Text()) );
		params.front()='{'; params.back() = '}';
		params = StringifyKeys( params );
		return json::parse( params );
	}
	DB::TableQL LoadTable( Parser& q, sv jsonName )noexcept(false)
	{
		var j = q.Peek()=="(" ? ParseJson( q ) : json{};
		THROW_IF( q.Next()!="{", Exception("Expected '{' after table name.") );
		DB::TableQL table{ string{jsonName}, j };
		for( auto token = q.Next(); token!="}"; token = q.Next() )
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
			results.push_back( LoadTable( q, jsonName) );
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
		 //{{ createUser(input:{{ name: \"{}\", target: \"{}\", description: \"{}\" }}){{ id }} }}
		THROW_IF( q.Next()!="{", Exception("mutation expected '{' as 1st character.") );
		var command = q.Next();
		uint iType=0;
		for( ;iType<DB::MutationQLStrings.size() && !command.starts_with(DB::MutationQLStrings[iType]); ++iType ); THROW_IF( iType==DB::MutationQLStrings.size(), Exception("Could not find mutation {}", command) );

		auto tableJsonName = string{ command.substr(DB::MutationQLStrings[iType].size()) };
		tableJsonName[0] = tolower( tableJsonName[0] );
		var type = (DB::EMutationQL)iType;
/*		string parentJsonName;
		if( type>DB::EMutationQL::Purge )
		{
			var childParent = StringUtilities::Split( string{tableJsonName}, "To" ); THROW_IF( childParent.size()!=2, Exception("could not find child/parent in {}", command) );
			tableJsonName = childParent[0];
			parentJsonName = childParent[1];
		}*/
		var j = ParseJson( q );
		var wantsResult  = q.Peek()=="{";
		optional<DB::TableQL> result = wantsResult ? LoadTable( q, tableJsonName ) : optional<DB::TableQL>{};
		DB::MutationQL ql{ string{tableJsonName}, type, j, result/*, parentJsonName*/ };
		return ql;
	}
	DB::RequestQL DB::ParseQL( sv query )noexcept(false)
	{
		uint i = query.find_first_of( "{" );
		THROW_IF( i==sv::npos || i>query.size()-2, Exception("Invalid query '{}'", query) );
		Parser parser{ query.substr(i+1), {'{', '}', '(', ')', ','} };
		auto name = parser.Next();
		if( name=="query" )
			name = parser.Next();

		return name=="mutation" ? DB::RequestQL{ LoadMutation(parser) } : DB::RequestQL{ LoadTables(parser,name) };
	}
}