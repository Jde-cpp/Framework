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

	void QueryTable( DB::TableQL table, uint userId, json& jData )noexcept(false)
	{
		var isPlural = table.JsonName.ends_with( "s" );
		var pSyntax = _pSyntax; THROW_IF( !pSyntax, Exception("SqlSyntax not set.") );
		TEST_ACCESS( "Read", table.DBName(), userId ); //TODO implement.
		ostringstream sql{ "select ", std::ios::ate };
		vector<string> jsonMembers; vector<uint> dates;
		var pSchemaTable = _schema.FindTableSuffix( table.DBName() ); THROW_IF( !pSchemaTable, Exception("Could not find table '{}' in schema", table.DBName()) );
		for( var& column : table.Columns )
		{
			if( jsonMembers.size() )
				sql << ", ";
			jsonMembers.emplace_back( column.JsonName );
			string columnName = DB::Schema::FromJson( column.JsonName );
			var pSchemaColumn = pSchemaTable->FindColumn( columnName ); THROW_IF( !pSchemaColumn, Exception("Could not find column '{}'", columnName) );
			if( pSchemaColumn->Type==DataType::DateTime )
			{
				sql << pSyntax->DateTimeSelect( columnName );
				dates.push_back( jsonMembers.size()-1 );
			}
			else
				sql << columnName;
		}
		sql << endl << "from\t" << pSchemaTable->Name;
		// for( var& table : table.Tables )
		// {

		// }
		std::vector<DB::DataValue> parameters; parameters.reserve( 8 );
		if( !table.Args.empty() )
		{
			sql << endl << "where\t";
			var begin = sql.tellp();
			for( var& [name,value] : table.Args.items() )
			{
				var columnName = DB::Schema::FromJson( name );
				var pColumn = pSchemaTable->FindColumn( columnName ); THROW_IF( !pColumn, Exception("column '{}' not found.", columnName) );
				if( begin != sql.tellp() )
					sql << " and ";
				sql << columnName << "=?";
				parameters.push_back( ToDataValue(pColumn->Type, value, name) );
			}
		}

		var jsonTableName = table.JsonName;// DB::Schema::ToJson( table.DBName() )
		auto fnctn = [&]( const DB::IRow& row )
		{
			json j;
			for( uint i=0; i<jsonMembers.size(); ++i )
			{
				DBG( jsonMembers[i] );
				DB::DataValue value = row[i];
				var index = (EDataValue)value.index();
				if( index==EDataValue::Null )
					continue;
				auto& member = j[jsonMembers[i]];
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
					if( find(dates.begin(), dates.end(), i)!=dates.end() )
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
			}
			if( isPlural )
			{
				if( !jData.contains(jsonTableName) )
					jData[jsonTableName] = json::array();
				json item;
				item[DB::Schema::ToSingular(jsonTableName)] = j;
				jData[jsonTableName].push_back( j );
			}
			else
				jData[jsonTableName] = j;
		};
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
	json ParseJson( Parser& q )
	{
		auto params = string{ q.Next(')') }; THROW_IF( params.front()!='(', Exception("Expected '(' vs {} @ '{}' to start function - '{}'.",  params.front(), q.Index()-1, q.Text()) );
		params.front()='{'; params.back() = '}';
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
				table.Tables.push_back( LoadTable(q, token) );
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