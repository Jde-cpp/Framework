#include "Table.h"
#include <jde/Str.h>
#include "../DataType.h"
#include "../Syntax.h"
#include "Schema.h"


#define var const auto

namespace Jde::DB
{
	using nlohmann::json;

	Column::Column( sv name, uint ordinalPosition, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isIdentity, bool isId, optional<uint> numericPrecision, optional<uint> numericScale )noexcept:
		Name{name},
		OrdinalPosition{ordinalPosition},
		Default{dflt},
		IsNullable{isNullable},
		Type{type},
		MaxLength{maxLength},
		IsIdentity{isIdentity},
		IsId{isId},
		NumericPrecision{numericPrecision},
		NumericScale{numericScale}
	{}

	Column::Column( sv name )noexcept:
		Name{ name }
	{}

	Column::Column( sv name, const nlohmann::json& j, const flat_map<string,Column>& commonColumns, const flat_map<string,Table>& parents, const nlohmann::ordered_json& schema )noexcept(false):
		Name{ name }
	{
		auto getType = [this,&commonColumns, &schema, &parents]( sv typeName )
		{
			IsNullable = typeName.ends_with( "?" );
			if( IsNullable )
				typeName = typeName.substr( 0, typeName.size()-1 );
			if( var p = commonColumns.find(string{typeName}); p!=commonColumns.end() )
			{
				var name = Name; var isNullable = IsNullable;
				*this = p->second;
				Name = name;
				IsNullable = isNullable;
			}
			else
			{
				Type = ToType( typeName );
				if( Type==EType::None )
				{
					if( var pPKTable = schema.find(string{typeName}); pPKTable!=schema.end() )
					{
						Table table{ typeName, *pPKTable, parents, commonColumns, schema };
						if( var pColumn = table.SurrogateKey.size()==1 ? table.FindColumn(table.SurrogateKey.front()) : nullptr; pColumn )
							Type = pColumn->Type;
						IsEnum = table.Data.size();
					}
					if( Type==EType::None )
						Type = EType::UInt;
					PKTable = Schema::FromJson( typeName );
				}
			}
		};
		if( j.is_object() )
		{
			if( j.contains("sequence") )
			{
				IsIdentity = j.find( "sequence" )->get<bool>();
				if( IsIdentity )
					IsNullable = Insertable = Updateable = false;
			}
			else
			{
				if( j.contains("type") )
					getType( j.find("type")->get<string>() );
				if( j.contains("length") )
				{
					MaxLength = j.find( "length" )->get<uint>();
					if( Type!=EType::Char )
						Type = EType::VarTChar;
				}
				if( j.contains("default") )
					Default = j.find("default")->get<string>();
				if( j.contains("insertable") )
					Insertable = j.find("insertable")->get<bool>();
				if( j.contains("updateable") )
					Updateable = j.find("updateable")->get<bool>();
			}
		}
		else if( j.is_string() )
			getType( j.get<string>() );
	}
	α Column::DataTypeString(const Syntax& syntax)const noexcept->string
	{
		return MaxLength ? format( "{}({})", ToString(Type, syntax), *MaxLength ) : ToString( Type, syntax );
	}
	α Column::Create( const Syntax& syntax )const noexcept->string
	{
		var null = IsNullable ? "null"sv : "not null"sv;
		const string sequence = IsIdentity ?  " "+string{syntax.IdentityColumnSyntax()} : string{};
		string dflt = Default.size() ? format( " default {}", Default=="$now" ? syntax.NowDefault() : format("'{}'"sv, Default) ) : string{};
		return format( "{} {} {}{}{}", Name, DataTypeString(syntax), null, sequence, dflt );
	}

	Table::Table( sv name, const nlohmann::json& j, const flat_map<string,Table>& parents, const flat_map<string,Column>& commonColumns, const nlohmann::ordered_json& schema ):
		Name{ name }
	{
		if( var& p = j.find("parent"); p != j.end() )
		{
			var parentName = p->get<string>();
			var pParent = parents.find( parentName ); THROW_IF( pParent==parents.end(), "Could not find '{}' parent '{}'", name, parentName );
			for( var& column : pParent->second.Columns )
				Columns.push_back( column );
			for( var& index : pParent->second.Indexes )
				Indexes.push_back( Index{index.Name, Name, index} );
			SurrogateKey = pParent->second.SurrogateKey;
		}
		for( var& [attribute,value] : j.items() )
		{
			if( attribute=="parent" )
				continue;
			if( attribute=="columns" )
			{
				for( var& it : value )
				{
					var dbName = Schema::FromJson( it["name"].get<string>() );
					//if( name=="dgr_classes" && dbName=="name" )
						//Dbg( "" );
					Columns.push_back( Column{dbName, it, commonColumns, parents, schema} );
					if( auto p=find(SurrogateKey.begin(), SurrogateKey.end(), dbName); p!=SurrogateKey.end() )
					{
						Columns.back().IsId = true;
						Columns.back().Updateable = false;
					}
				}
			}
			else if( attribute=="surrogateKey" )
			{
				for( var& it : value )
				{
					const string colName{ Schema::FromJson(it.get<string>()) };
					SurrogateKey.push_back( colName );
					if( auto p=find_if(Columns.begin(), Columns.end(), [&colName](var& x){return x.Name==colName;} ); p!=Columns.end() )
						p->IsId = true;
				}
				Indexes.push_back( Index{"pk", Name, true, &SurrogateKey} );
			}
			else if( attribute=="naturalKey" )
			{
				vector<SchemaName> columns;
				for( var& it : value )
					columns.push_back( DB::Schema::FromJson(it.get<string>()) );
				var name2 = NaturalKeys.empty() ? "nk" : format( "nk{}", NaturalKeys.size() );
				Indexes.push_back( Index{name2, Name, false, &columns} );
				NaturalKeys.push_back( columns );
			}
			else if( attribute=="flagsData" )
			{
				for( var& it : value )
					FlagsData.emplace( FlagsData.empty() ? 0 : 1 << (FlagsData.size()-1), it );
			}
			else if( attribute=="data" )
			{
				auto seed = 0;
				for( var& it : value )
				{
					if( it.is_string() )
					{
						json jRow;
						jRow["id"] = seed++;
						jRow["name"] = it.get<string>();
						Data.push_back( jRow );
					}
					else
						Data.push_back( it );
				}
			}
			else if( attribute=="customInsertProc" )
				CustomInsertProc = value.get<bool>();
			else if( attribute!="usePrefix" )
				ASSERT( false );
		}
	}

	α Table::InsertProcName()const noexcept->string
	{
		var haveSequence = std::find_if( Columns.begin(), Columns.end(), [](var& c){return c.IsIdentity;} )!=Columns.end();
		return !haveSequence || CustomInsertProc ? string{} : format( "{}_insert", DB::Schema::ToSingular(Name) );
	}

	α Table::InsertProcText( const Syntax& syntax )const noexcept->string
	{
		ostringstream osCreate, osInsert, osValues;
		osCreate << "create procedure " << InsertProcName() << "(";
		osInsert << "\tinsert into " << Name << "(";
		osValues << "\t\tvalues(";
		var prefix = syntax.ProcParameterPrefix().empty() ? "_"sv : syntax.ProcParameterPrefix();
		char delimiter = ' ';
		for( var& column : Columns )
		{
			string value = format( "{}{}"sv, prefix, column.Name );
			if( column.Insertable )
				osCreate << delimiter << prefix << column.Name  << " " << column.DataTypeString( syntax );
			else
			{
				 if( column.IsNullable || column.Default.empty() )
				 	continue;
				if( column.Default=="$now" )
					value = syntax.UtcNow();
			}
			osInsert << delimiter << column.Name;
			osValues << delimiter  << value;
			delimiter = ',';
		}
		osInsert << " )" << endl;
		osValues << " );" << endl;
		osCreate << " )" << endl << syntax.ProcStart() << endl;
		osCreate << osInsert.str() << osValues.str();
		osCreate << "\tselect " << syntax.IdentitySelect() <<";" << endl << syntax.ProcEnd() << endl;// into _id
		return osCreate.str();
	}

	α Table::Create( const Syntax& syntax )const noexcept->string
	{
		ostringstream createStatement;
		createStatement << "Create table " << Name << "(" << endl;
		string suffix = "";
		string pk;
		for( var& column : Columns )
		{
			if( column.IsIdentity )
				pk = format( "PRIMARY KEY({})"sv, column.Name );
			createStatement << suffix << endl << "\t" << column.Create( syntax );
			suffix = ",";
		}
		if( pk.size() )
			createStatement << suffix << endl << "\t" << pk << endl;
		createStatement << ")";
		return createStatement.str();
	}

	Index::Index( sv indexName, sv tableName, bool primaryKey, vector<CIString>* pColumns, bool unique, optional<bool> clustered )noexcept:
		Name{ indexName },
		TableName{ tableName },
		Columns{ pColumns ? *pColumns : vector<CIString>{} },
		Clustered{ clustered ? *clustered : primaryKey },
		Unique{ unique },
		PrimaryKey{ primaryKey }
	{}
	Index::Index( sv indexName, sv tableName, const Index& y )noexcept:
		Name{ indexName },
		TableName{ tableName },
		Columns{ y.Columns },
		Clustered{ y.Clustered },
		Unique{ y.Unique },
		PrimaryKey{ y.PrimaryKey }
	{}

	α Index::Create( sv name, sv tableName, const Syntax& syntax )const noexcept->string
	{
		string unique = Unique ? "unique" : "";
		ostringstream os;
		if( PrimaryKey )
			os << "alter table " << tableName << " add constraint " << name << (Clustered || !syntax.SpecifyIndexCluster() ? "" : " nonclustered") << " primary key(";
		else
			os << "create " << (Clustered && syntax.SpecifyIndexCluster() ? "clustered " : " ") << unique << " index "<< name << " on " << endl << tableName << "(";

		os << Str::AddCommas( Columns ) << ")";

		return os.str();
	}

	α ForeignKey::Create( sv name, sv columnName, const DB::Table& pkTable, sv foreignTable )noexcept(false)->string
	{
		THROW_IF( pkTable.SurrogateKey.size()!=1, "{} has {} columns in pk, !1 has not implemented", pkTable.Name, pkTable.SurrogateKey.size() );
		ostringstream os;
		os << "alter table " << foreignTable << " add constraint " << name << " foreign key(" << columnName << ") references " << pkTable.Name << "(" << Str::AddCommas(pkTable.SurrogateKey) << ")";
		return os.str();
	}
	const Column* Table::FindColumn( sv name )const noexcept
	{
		auto pColumn = find_if( Columns.begin(), Columns.end(), [&name](var& c){return c.Name==name;} );
		return pColumn==Columns.end() ? nullptr : &(*pColumn);
	}

	α ColumnStartingWith( const Table& table, sv part )->string
	{
		auto pColumn = find_if( table.Columns.begin(), table.Columns.end(), [&part](var& c){return c.Name.starts_with(part);} );
		return pColumn==table.Columns.end() ? string{} : pColumn->Name;
	}

	α TableNamePart( const Table& table, uint8 index )noexcept(false)->string
	{
		var nameParts = Str::Split( table.NameWithoutType(), '_' );
		return nameParts.size()>index ? DB::Schema::ToSingular( nameParts[index] ) : string{};
	}
	α Table::Prefix()const noexcept->string{ return TableNamePart(*this, 0); }
	α Table::NameWithoutType()const noexcept->string{ var underscore = Name.find_first_of('_'); return Name.substr(underscore==string::npos ? 0 : underscore+1); }

	α Table::FKName()const noexcept->string{ return Schema::ToSingular(NameWithoutType())+"_id"; }
	α Table::JsonTypeName()const noexcept->string
	{
		auto name = Schema::ToJson( Schema::ToSingular(NameWithoutType()) );
		if( name.size() )
			name[0] = (char)std::toupper( name[0] );
		return name;
	}
	α Table::ChildId()const noexcept(false)->string
	{
		var part = TableNamePart( *this, 0 );
		return part.empty() ? part : ColumnStartingWith( *this, part );
	}

	α Table::ChildTable( const DB::Schema& schema )const noexcept(false)->sp<const Table>
	{
		var part = TableNamePart( *this, 0 );
		return part.empty() ? sp<const Table>{} : schema.TryFindTableSuffix( Schema::ToPlural(part) );
	}

	α Table::ParentId()const noexcept(false)->string
	{
		var part = TableNamePart( *this, 1 );
		return part.empty() ? part : ColumnStartingWith( *this, part );
	}

	α Table::ParentTable( const DB::Schema& schema )const noexcept(false)->sp<const Table>
	{
		var part = TableNamePart( *this, 1 );
		return part.empty() ? sp<const Table>{} : schema.TryFindTableSuffix( Schema::ToPlural(part) );
	}
}