#include "GraphQuery.h"
#include "Database.h"
#include "GraphQL.h"
#include "Syntax.h"

#define var const auto
namespace Jde::DB
{
	static const LogTag& _logLevel = Logging::TagLevel( "ql" );
	α Where( const DB::TableQL& table, const Table& schemaTable, vector<object>& parameters )noexcept(false)->string
	{
		LOG( "Where({})", table.Args.dump() );
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

	α GraphQL::Query( const DB::TableQL& table, json& jData )noexcept(false)->void
	{
		var isPlural = table.JsonName.ends_with( "s" );
		var pSyntax = DB::DefaultSyntax();// _pSyntax; THROW_IF( !pSyntax, Exception("SqlSyntax not set.") );
		var pSchemaTable = Schema().FindTableSuffix( table.DBName() ); THROW_IF( !pSchemaTable, "Could not find table '{}' in schema", table.DBName() );
		vector<tuple<string,string>> jsonMembers;
		function<string(const DB::TableQL&, const DB::Table& dbTable, sp<const DB::Table> pDefTable, vector<uint>&, flat_map<uint,sp<const DB::Table>>&, sv, bool, uint*, string*, vector<tuple<string,string>>* )> columnSql;
		columnSql = [pSyntax, &columnSql]( const DB::TableQL& table2, const DB::Table& dbTable, sp<const DB::Table> pDefTable, vector<uint>& dates, flat_map<uint,sp<const DB::Table>>& flags, sv defaultPrefix, bool excludeId, uint* pIndex, string* pSubsequentJoin, vector<tuple<string,string>>* pJsonMembers )
		{
			uint index2=0;
			if( !pIndex )
				pIndex = &index2;
			vector<string> columns;
			for( var& column : table2.Columns )
			{
				auto columnName = DB::Schema::FromJson( column.JsonName );
				if( columnName=="id" && excludeId )
					continue;
				auto findColumn = []( const DB::Table& t, str c ){ auto p = t.FindColumn( c ); if( !p ) p = t.FindColumn( c+"_id" ); return p; }; //+_id for enums
				auto pSchemaColumn = findColumn( dbTable, columnName );
				if( !pDefTable && pSchemaColumn && pSchemaColumn->PKTable.size() )//enumeration pSchemaColumn==authenticator_id
				{
					auto p = Schema().Tables.find( pSchemaColumn->PKTable ); CHECK( p!=Schema().Tables.end() );//um_authenticators
					pDefTable = p->second;
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
						var p = Schema().Tables.find( pSchemaColumn->PKTable ); THROW_IF( p==Schema().Tables.end(), "Could not find flags pk table for {}.{}", pDefTable->Name, columnName );
						flags.emplace( *pIndex, p->second );
					}
					columnName = format( "{}.{}", pDefTable->Name, pSchemaColumn->Name );
				}
				else if( pSchemaColumn )
				{
					columnName = format( "{}.{}", defaultPrefix, pSchemaColumn->Name );
				}
				THROW_IF( !pSchemaColumn, "Could not find column '{}.{}'", pDefTable->Name, columnName );//Could not find column 'um_role_permissions.api'" thrown in the test body

				var dateTime = pSchemaColumn->Type==EType::DateTime;
				if( dateTime )
					dates.push_back( *pIndex );
				columns.push_back( dateTime ? pSyntax->DateTimeSelect(columnName) : columnName );
				if( pJsonMembers )
					pJsonMembers->push_back( make_tuple(table2.JsonName, column.JsonName) );//DB::Schema::ToJson(pSchemaColumn->Name))

				++(*pIndex);
			}
			for( var& childTable : table2.Tables )
			{
				var childDbName = childTable->DBName();
				var pPKTable = Schema().FindTableSuffix( childDbName ); THROW_IF( !pPKTable, "Could not find table '{}' in schema", childDbName );
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
		vector<uint> dates; flat_map<uint,sp<const DB::Table>> flags;
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
		vector<object> parameters;
		var where = Where( table, *pSchemaTable, parameters );
		auto colToJson = [&]( const DB::IRow& row, uint iColumn, json& obj, sv objectName, sv memberName, const vector<uint>& dates, const flat_map<uint,sp<flat_map<uint,string>>>& flagValues )
		{
			object value = row[iColumn];
			var index = (EObject)value.index();
			if( index==EObject::Null )
				return;
			auto& member = objectName.empty() ? obj[string{memberName}] : obj[string{objectName}][string{memberName}];
			if( index==EObject::String )
				member = get<string>( value );
			else if( index==EObject::StringView )
				member = get<sv>( value );
			else if( index==EObject::Bool )
				member = get<bool>( value );
			else if( index==EObject::Int64 )
			{
				var intValue = get<_int>( value );
				if( find(dates.begin(), dates.end(), iColumn)!=dates.end() )
					member = Jde::DateTime( intValue ).ToIsoString();
				else
					member = get<_int>( value );
			}
			else if( index==EObject::Uint || index==EObject::Int )
			{
				if( var pFlagValues = flagValues.find(iColumn); pFlagValues!=flagValues.end() )
				{
					member = json::array();
					uint remainingFlags = index==EObject::Uint ? get<uint>( value ) : get<int>( value );
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
				else if( index==EObject::Uint )
					member = get<uint>( value );
				else
					member = get<int>( value );
			}
			//else if( index==EObject::Decimal2 )
			//	member = (float)get<Decimal2>( value );
			else if( index==EObject::Double )
				member = get<double>( value );
			//else if( index==EObject::DoubleOptional )
			//	member = get<optional<double>>(value).value();
			else if( index==EObject::Time )
				member = ToIsoString( get<DB::DBTimePoint>(value) );
			else if( index==EObject::StringPtr )
			{
				if( get<sp<string>>(value) )
					member = *get<sp<string>>(value);
			}
			else
				ERR( "{} not implemented"sv, (uint8)index );
		};
		flat_map<string,flat_multimap<uint,json>> subTables;
		auto selectSubTables = [&subTables, &columnSql, &colToJson, &parameters]( const vector<sp<const DB::TableQL>>& tables, const DB::Table& parentTable, sv where2 )
		{
			for( var& pQLTable : tables )
			{
				auto pSubTable = Schema().FindTableSuffix( pQLTable->DBName() ); THROW_IF( !pSubTable, "Could not find table '{}' in schema", pQLTable->DBName() );
				sp<const DB::Table> pDefTable;
				if( pSubTable->IsMap() )//for RolePermissions, subTable=Permissions, defTable=RolePermissions
				{
					pDefTable = pSubTable;
					var subIsParent = pDefTable->ChildId()==parentTable.FKName();
					pSubTable = subIsParent ? pDefTable->ParentTable( Schema() ) : pDefTable->ChildTable( Schema() ); THROW_IF( !pSubTable, "Could not find {} table for '{}' in schema", (subIsParent ? "parent" : "child"), pDefTable->Name );
				}
				else
					pDefTable = Schema().FindDefTable( parentTable, *pSubTable );
				if( !pDefTable )
					continue;  //'um_permissions<->'apis' //THROW_IF( !pDefTable, Exception("Could not find def table '{}<->'{}' in schema", parentTable.Name, pQLTable->DBName()) );

				var defTableName = pDefTable->Name;
				ostringstream subSql{ format("select {0}.{1} primary_id, {0}.{2} sub_id", defTableName, parentTable.FKName(), pSubTable->FKName()), std::ios::ate };
				var& subTableName = pSubTable->Name;
				vector<uint> subDates; flat_map<uint,sp<const DB::Table>> subFlags;
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
					subFlagValues[index+2] = Future<flat_map<uint,string>>( DataSource()->SelectEnum<uint>(pTable->Name) ).get();
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
							colToJson( row, i, jRow, "", column.JsonName, subDates, subFlagValues );
						}
					};
					rowToJson2( pQLTable->Columns, true, jSubRow, index );
					for( var& childTable : pQLTable->Tables )
					{
						var childDbName = childTable->DBName();
						var pPKTable = Schema().FindTableSuffix( childDbName ); THROW_IF( !pPKTable, "Could not find table '{}' in schema", childDbName );
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
				DataSource()->Select( subSql.str(), forEachRow, parameters );
			}
		};
		selectSubTables( table.Tables, *pSchemaTable, where );
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
				json jRow;
				for( uint i=0; i<jsonMembers.size(); ++i )
				{
					var parent = get<0>(jsonMembers[i]);
					colToJson( row, i, jRow, parent==jsonTableName ? sv{} : parent, get<1>(jsonMembers[i]), dates, {} );
				}
				if( subTables.size() )
					addSubTables( jRow );
				if( isPlural )
					jData[jsonTableName].push_back( jRow );
				else
					jData[jsonTableName] = jRow;
			};
			DataSource()->Select( sql.str(), primaryRow, parameters );
		}
		else
		{
			json jRow;
			addSubTables( jRow );
			jData[jsonTableName] = jRow;
		}
	}
}