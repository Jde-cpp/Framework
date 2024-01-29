#include "GraphQuery.h"
#include "Database.h"
#include "GraphQL.h"
#include "Syntax.h"

#define var const auto
namespace Jde::DB
{
	using std::endl;
	static sp<LogTag> _logTag = Logging::Tag( "ql" );
	#define  _schema DB::DefaultSchema()
	α Where( const DB::TableQL& table, const Table& schemaTable, vector<object>& parameters )ε->string
	{
		TRACE( "Where({})", table.Args.dump() );
		var pWhere = table.Args.find( "filter" );
		var j = pWhere==table.Args.end() ? table.Args : *pWhere;
		ostringstream where;
		for( var& [name,value] : j.items() )
		{
			var columnName = DB::Schema::FromJson( name );
			var pColumn = schemaTable.FindColumn( columnName ); THROW_IF( !pColumn, "column '{}' not found.", columnName );
			if( where.tellp()!=std::streampos(0) )
				where << endl << "\tand ";
			where << schemaTable.Name << "." << columnName;
			sv op = "=";
			const json* pJson{ nullptr };
			if( value.is_string() || value.is_number() )
				pJson = &value;
			else if( value.is_null() )
				where << " is null";
			else if( value.is_object() && value.items().begin()!=value.items().end() )
			{
				var first = value.items().begin();
				if( first.value().is_null() )
					where << " is " << (first.key()=="eq" ? "" : "not ") << "null";
				else
				{
					if( first.key()=="ne" )
						op = "!=";
					pJson = &first.value();
				}
			}
			if( pJson )
			{
				where << op << "?";
				parameters.push_back( ToObject(pColumn->Type, *pJson, name) );
			}
		}
		return where.str();
	}
#define _db DataSource()
#define _syntax DB::DefaultSyntax()
	α AddColumn( const ColumnQL& c, const DB::TableQL& qlTable, const DB::Table& dbTable, vector<string>& columns, const DB::Table* pDefTable, vector<uint>& dates, flat_map<uint,sp<const DB::Table>>& flags, sv defaultPrefix, bool excludeId, uint* pIndex, string* pSubsequentJoin, vector<tuple<string,string>>* pJsonMembers )->void
	{
		auto columnName = DB::Schema::FromJson( c.JsonName );
		if( columnName=="id" && excludeId )
			return;
		auto findColumn = []( const DB::Table& t, sv n ){ auto p = t.FindColumn( n ); if( !p ) p = t.FindColumn( string{n}+"_id" ); return p; }; //+_id for enums
		auto pSchemaColumn = findColumn( dbTable, columnName );
		if( !pDefTable && pSchemaColumn && pSchemaColumn->PKTable.size() )//enumeration pSchemaColumn==authenticator_id
		{
			auto p = _schema.Tables.find( pSchemaColumn->PKTable ); CHECK( p!=_schema.Tables.end() );//um_authenticators
			pDefTable = &*p->second;
			var fkName = pSchemaColumn->Name;
			var join = pSchemaColumn->IsNullable ? "left "sv : sv{};
			auto pOther = findColumn( *pDefTable, DB::Schema::ToSingular("name") ); CHECK( pOther );
			columnName = format( "{}.{} {}", pDefTable->Name, pOther->Name, columnName );
			*pSubsequentJoin += format( "\t{0}join {1} on {1}.id={2}.{3}"sv, join, pDefTable->Name, defaultPrefix, fkName );
		}
		else if( pDefTable && !pSchemaColumn )
		{
			pSchemaColumn = findColumn( *pDefTable, columnName );
			if( !pSchemaColumn )
			{
				pSchemaColumn = findColumn( *pDefTable, DB::Schema::ToSingular(columnName) ); THROW_IF( !pSchemaColumn, "Could not find column '{}.{}'", pDefTable->Name, columnName );
				var p = _schema.Tables.find( pSchemaColumn->PKTable ); THROW_IF( p==_schema.Tables.end(), "Could not find flags pk table for {}.{}", pDefTable->Name, columnName );
				flags.emplace( columns.size(), p->second );//*pIndex
			}
			columnName = format( "{}.{}", pDefTable->Name, pSchemaColumn->Name );
		}
		else if( pSchemaColumn )
		{
			if( pSchemaColumn->IsEnum )
			{
				c.SchemaColumnPtr = pSchemaColumn;
				var p = _schema.Tables.find( pSchemaColumn->PKTable ); THROW_IF( p==_schema.Tables.end(), "Could not find flags pk table for {}.{}", pDefTable->Name, columnName );
				flags.emplace( *pIndex, p->second );
			}
			else if( pSchemaColumn->QLAppend.size() && !qlTable.FindColumn(pSchemaColumn->QLAppend) )
			{
				if( CIString name{ format("{}.{}", defaultPrefix, DB::Schema::FromJson(pSchemaColumn->QLAppend)) }; std::find(columns.begin(), columns.end(), name)==columns.end() )
				{
					columns.push_back( name );//add db column, doesn't add qlColumn
					c.SchemaColumnPtr = pSchemaColumn;
					c.SchemaColumnPtr->TablePtr = &dbTable;
				}
			}

			columnName = format( "{}.{}", defaultPrefix, pSchemaColumn->Name );
		}
		THROW_IF( !pSchemaColumn, "Could not find column '{}.{}'", pDefTable->Name, columnName );//Could not find column 'um_role_permissions.api'" thrown in the test body

		var dateTime = pSchemaColumn->Type==EType::DateTime;
		if( dateTime )
			dates.push_back( *pIndex );
		columns.push_back( dateTime ? _syntax.DateTimeSelect(columnName) : columnName );
		if( pJsonMembers )
			pJsonMembers->push_back( make_tuple(qlTable.JsonName, c.JsonName) );//DB::Schema::ToJson(pSchemaColumn->Name))

		++(*pIndex);
	}

	α ColumnSql( const DB::TableQL& qlTable, const DB::Table& dbTable, const DB::Table* pDefTable, vector<uint>& dates, flat_map<uint,sp<const DB::Table>>& flags, sv defaultPrefix, bool excludeId, uint* pIndex, string* pSubsequentJoin, vector<tuple<string,string>>* pJsonMembers )->string
	{
		uint index2 = 0;
		if( !pIndex )
			pIndex = &index2;
		vector<string> columns;
		for( var& c : qlTable.Columns )
			AddColumn( c, qlTable, dbTable, columns, pDefTable, dates, flags, defaultPrefix, excludeId, pIndex, pSubsequentJoin, pJsonMembers );

		for( var& childTable : qlTable.Tables )
		{
			var childDbName = childTable->DBName();
			var& pkTable = _schema.FindTableSuffix( childDbName );
			if( pSubsequentJoin )
			{
				const DB::Column* pColumn = nullptr;
				auto findFK = [&pkTable](var& x){ return x.PKTable==pkTable.Name; };
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
					columns.push_back( ColumnSql(*childTable, pkTable, nullptr, dates, flags, pkTable.Name, false, pIndex, nullptr, pJsonMembers) );
					*pSubsequentJoin += format( "\tleft join {0} on {0}.id={1}.{2}", pkTable.Name, childPrefix, pColumn->Name );
				}
			}
		}
		return Str::AddCommas( columns );
	}

	α GraphQL::Query( const DB::TableQL& table, json& jData, UserPK userId )ε->void
	{
		ASSERT(_db);
		var isPlural = table.JsonName.ends_with( "s" );
		var& schemaTable = _schema.FindTableSuffix( table.DBName() );
		vector<tuple<string,string>> jsonMembers;
		vector<uint> dates; flat_map<uint,sp<const DB::Table>> flags;
		string joins;
		var columnSqlValue = ColumnSql( table, schemaTable, nullptr, dates, flags, schemaTable.Name, false, nullptr, &joins, &jsonMembers );
		ostringstream sql;
		if( columnSqlValue.size() )
			sql << "select " << columnSqlValue;

		var addId = table.Tables.size() && !table.FindColumn("id") && table.Columns.size();
		if( addId )
			sql << ", id";
		var& tableName = schemaTable.Name;
		if( sql.tellp()!=std::streampos(0) )
		{
			sql << endl << "from\t" << tableName;
			if( joins.size() )
				sql << endl << joins;
		}
		vector<object> parameters;
		var where = Where( table, schemaTable, parameters );
		auto pAuthorizer = UM::FindAuthorizer( tableName );
		auto colToJson = [&]( const DB::IRow& row, uint iColumn, json& obj, sv objectName, sv memberName, const vector<uint>& dates, const flat_map<uint,sp<flat_map<uint,string>>>& flagValues, const DB::ColumnQL* pMember=nullptr )->bool
		{
			var value{ row[iColumn] };
			if( var index{(EObject)value.index()}; index!=EObject::Null )
			{
				auto& m = objectName.empty() ? obj[string{memberName}] : obj[string{objectName}][string{memberName}];
				if( index==EObject::UInt64 || index==EObject::Int32 )
				{
					if( var pFlagValues = flagValues.find(iColumn); pFlagValues!=flagValues.end() )
					{
						var isEnum{ pMember && pMember->SchemaColumnPtr && pMember->SchemaColumnPtr->IsEnum };
						if( !isEnum )
							m = json::array();
						uint remainingFlags = ToUInt( value );
						for( uint iFlag=0x1; remainingFlags!=0; iFlag <<= 1 )
						{
							if( (remainingFlags & iFlag)==0 )
								continue;
							if( var pValue = pFlagValues->second->find(iFlag); pValue!=pFlagValues->second->end() )
							{
								if( isEnum )
									m = pValue->second;
								else
									m.push_back( pValue->second );
							}
							else
								m.push_back( std::to_string(iFlag) );
							remainingFlags -= iFlag;
						}
					}
					else if( pAuthorizer && memberName=="id" && !pAuthorizer->CanRead(userId, (UserPK)ToUInt(value)) )//TODO move to sql
						return false;//TODO uncomment
/*					else if( pMember && pMember->SchemaColumnPtr && pMember->SchemaColumnPtr->IsEnum )
					{
						if( var pFlagValues = flagValues.find(iColumn); pFlagValues!=flagValues.end() )
						{
							if( var pValue = pFlagValues->second->find(ToInt(value)); pValue!=pFlagValues->second->end() )
								m.push_back( pValue->second );
							else
								ASSERT( false );
						}
						else
							ASSERT( false );
					}*/
					else
						ToJson( value, m );
				}
				else if( index==EObject::Int64 && find(dates.begin(), dates.end(), iColumn)!=dates.end() )
					m = Jde::DateTime( get<_int>(value) ).ToIsoString();
				else
					ToJson( value, m );
			}
			return true;
		};
		flat_map<string,flat_multimap<uint,json>> subTables;
		auto selectSubTables = [&subTables, &colToJson, &parameters]( const vector<sp<const DB::TableQL>>& tables, const DB::Table& parentTable, sv where2 )
		{
			for( auto& pQLTable : tables )
			{
				auto pSubTable = &_schema.FindTableSuffix( pQLTable->DBName() );
				const DB::Table* pDefTable;
				if( pSubTable->IsMap() )//for RolePermissions, subTable=Permissions, defTable=RolePermissions
				{
					pDefTable = pSubTable;
					var subIsParent = pDefTable->ChildId()==parentTable.FKName();
					auto p = subIsParent ? pDefTable->ParentTable( _schema ) : pDefTable->ChildTable( _schema ); THROW_IF( !p, "Could not find {} table for '{}' in schema", (subIsParent ? "parent" : "child"), pDefTable->Name );
					pSubTable = &*p;
				}
				else
					pDefTable = &*_schema.FindDefTable( parentTable, *pSubTable );
				if( !pDefTable )
					continue;  //'um_permissions<->'apis' //THROW_IF( !pDefTable, Exception("Could not find def table '{}<->'{}' in schema", parentTable.Name, pQLTable->DBName()) );
				var subTable = *pSubTable;
				var defTableName = pDefTable->Name;
				ostringstream subSql{ format("select {0}.{1} primary_id, {0}.{2} sub_id", defTableName, parentTable.FKName(), subTable.FKName()), std::ios::ate };
				var& subTableName = subTable.Name;
				vector<uint> subDates; flat_map<uint,sp<const DB::Table>> subFlags;
				string subsequentJoin;
				var columns = ColumnSql( *pQLTable, subTable, pDefTable, subDates, subFlags, subTableName, true, nullptr, &subsequentJoin, nullptr );//rolePermissions
				if( columns.size() )
					subSql << ", " << columns;
				subSql << "\nfrom\t" << parentTable.Name << endl
					<< "\tjoin " << defTableName << " on " << parentTable.Name <<".id=" << defTableName << "." << parentTable.FKName();
				if( columns.size() )
				{
					subSql << "\tjoin " << subTableName << " on " << subTableName <<".id=" << defTableName << "." << subTable.FKName();
					if( subsequentJoin.size() )
						subSql << endl << subsequentJoin;
				}

				if( where2.size() )
					subSql << endl << "where\t" << where2;
				flat_map<uint,sp<flat_map<uint,string>>> subFlagValues;
				for( var& [index,pTable] : subFlags )
				{
					SelectCacheAwait<flat_map<uint,string>> a = _db->SelectEnum<uint>( pTable->Name );
					subFlagValues[index+2] = SFuture<flat_map<uint,string>>( move(a) ).get();
				}
				auto& rows = subTables.emplace( pQLTable->JsonName, flat_multimap<uint,json>{} ).first->second;
				auto forEachRow = [&]( const DB::IRow& row )
				{
					json jSubRow;
					uint index = 0;
					var rowToJson2 = [&row, &subDates, &colToJson, &subFlagValues]( const vector<DB::ColumnQL>& columns, bool checkId, json& jRow, uint& index2 )
					{
						for( var& column : columns )
						{
							auto i = checkId && column.JsonName=="id" ? 1 : (index2++)+2;
							if( column.SchemaColumnPtr && column.SchemaColumnPtr->QLAppend.size() && column.SchemaColumnPtr->TablePtr )
							{
								var pk = ToUInt( row[i++] ); ++index2;
								var pColumn = column.SchemaColumnPtr->TablePtr->FindColumn( DB::Schema::FromJson(column.SchemaColumnPtr->QLAppend) );  CHECK( pColumn && pColumn->IsEnum );
								var pEnum = SelectEnumSync( pColumn->PKTable ); CHECK( pEnum->find(pk)!=pEnum->end() );
								colToJson( row, i, jRow, "", column.JsonName, subDates, subFlagValues, &column );
								var name = jRow[column.JsonName].is_null() ? string{} : jRow[column.JsonName].get<string>();
								jRow[column.JsonName] = name.empty() ? pEnum->find(pk)->second : format( "{}\\{}", pEnum->find(pk)->second, name );
							}
							else
								colToJson( row, i, jRow, "", column.JsonName, subDates, subFlagValues, &column );
						}
					};
					rowToJson2( pQLTable->Columns, true, jSubRow, index );
					for( var& childTable : pQLTable->Tables )
					{
						var childDbName = childTable->DBName();
						var& pkTable = _schema.FindTableSuffix( childDbName );
						var pColumn = find_if( subTable.Columns.begin(), subTable.Columns.end(), [&pkTable](var& x){ return x.PKTable==pkTable.Name; } );
						if( pColumn!=subTable.Columns.end() )
						{
							json jChildTable;
							rowToJson2( childTable->Columns, false, jChildTable, index );
							jSubRow[childTable->JsonName] = jChildTable;
						}
					}
					rows.emplace( row.GetUInt(0), jSubRow );
				};
				_db->Select( subSql.str(), forEachRow, parameters );// select um_role_permissions.role_id
			}
		};
		selectSubTables( table.Tables, schemaTable, where );
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
			if( where.size() )
				sql << endl << "where\t" << where;
			if( isPlural )
				jData[jsonTableName] = json::array();
			auto primaryRow = [&]( const DB::IRow& row )
			{
				json jRow; bool authorized=true;
				for( uint i=0; i<jsonMembers.size() && authorized; ++i )
				{
					var [parent, column] = jsonMembers[i];
					if( var pColumn = table.JsonName==parent ? table.FindColumn(column) : nullptr; pColumn && pColumn->SchemaColumnPtr && pColumn->SchemaColumnPtr->QLAppend.size() )
					{
						var pk = ToUInt( row[i++] );
						var pAppend = pColumn->SchemaColumnPtr->TablePtr->FindColumn( DB::Schema::FromJson(pColumn->SchemaColumnPtr->QLAppend) );  CHECK( pAppend && pAppend->IsEnum );
						var pEnum = SelectEnumSync( pAppend->PKTable ); CHECK( pEnum->find(pk)!=pEnum->end() );
						colToJson( row, i, jRow, {}, column, dates, {} );
						var name = jRow[column].is_null() ? string{} : jRow[column].get<string>();
						jRow[column] = name.empty() ? pEnum->find(pk)->second : format( "{}\\{}", pEnum->find(pk)->second, name );

					}
					else
					//var column = get<1>( jsonMembers[i] );
						authorized = colToJson( row, i, jRow, parent==jsonTableName ? sv{} : parent, column, dates, {} );
				}
				if( authorized )
				{
					if( subTables.size() )
						addSubTables( jRow );
					if( isPlural )
						jData[jsonTableName].push_back( jRow );
					else
						jData[jsonTableName] = jRow;
				}
			};
			_db->Select( sql.str(), primaryRow, parameters );
		}
		else
		{
			json jRow;
			addSubTables( jRow );
			jData[jsonTableName] = jRow;
		}
	}
}