#include "Table.h"
#include "../../StringUtilities.h"
#include "../DataType.h"
#include "../Syntax.h"
#include "Schema.h"


#define var const auto

namespace Jde::DB
{
	using nlohmann::json;

	Column::Column( sv name, uint ordinalPosition, sv dflt, bool isNullable, DataType type, optional<uint> maxLength, bool isIdentity, bool isId, optional<uint> numericPrecision, optional<uint> numericScale )noexcept:
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
		Name{ name }//,.ends_with("?") ? name.substr(0,name.size()-1) : name
		//IsNullable{ name.ends_with("?") }
	{}

	Column::Column( sv name, const nlohmann::json& j, const flat_map<string,Column>& commonColumns )noexcept(false):
		Name{ name }
	{
		auto getType = [this,&commonColumns]( sv typeName )
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
				Type = ToDataType( typeName );
				if( Type==DataType::None )
				{
					Type = DataType::UInt;
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
					Type = DataType::VarTChar;
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
	string Column::DataTypeString()const noexcept
	{
		return MaxLength ? format( "{}({})", ToString(Type), *MaxLength ) : ToString( Type );
	}
	string Column::Create( const Syntax& syntax )const noexcept
	{
		var null = IsNullable ? "null"sv : "not null"sv;
		const string sequence = IsIdentity ?  " "+string{syntax.IdentityColumnSyntax()} : string{};
		string dflt;
		if( Default.size() && Default!="$now" )
		{
			auto value = format( "'{}'", Default );
			dflt = format( " default {}", value );
		}
		return format( "{} {} {}{}{}", Name, DataTypeString(), null, sequence, dflt );
	}

	Table::Table( sv name, const nlohmann::json& j, const flat_map<string,Table>& parents, const flat_map<string,Column>& commonColumns ):
		Name{ name }
	{
		//if( name=="um_role_permissions" )
		//	DBG( name );
		for( var& [columnName,value] : j.items() )
		{
			if( columnName=="$parent" )
			{
				var name2 = value.get<string>();
				var pParent = parents.find( j.find("$parent")->get<string>() ); THROW_IF( pParent==parents.end(), Exception("Could not find parent '{}'", j.find("$parent")->get<string>()) );
				for( var& column : pParent->second.Columns )
				{
					Columns.push_back( column );
//					DBG( "{}.{}, dt={}"sv, name, column.Name, Columns.back().DataTypeString() );
				}
				for( var& index : pParent->second.Indexes )
					Indexes.push_back( Index{index.Name, Name, index} );
				SurrogateKey = pParent->second.SurrogateKey;
			}
			else if( columnName=="$surrogateKey" )
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
			else if( columnName=="$naturalKey" )
			{
				vector<string> columns;
				for( var& it : value )
					columns.push_back( DB::Schema::FromJson(it.get<string>()) );
				var name = NaturalKeys.empty() ? "nk" : format( "nk{}", NaturalKeys.size() );
				Indexes.push_back( Index{name, Name, false, &columns} );
				NaturalKeys.push_back( columns );
			}
			else if( columnName=="$flagsData" )
			{
				for( var& it : value )
					FlagsData.emplace( FlagsData.empty() ? 0 : 1 << (FlagsData.size()-1), it );
			}
			else if( columnName=="$data" )
			{
				//for( json::iterator it = value.begin(); it != value.end(); ++it )
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
			else
			{
				var dbName = Schema::FromJson( columnName );
				Columns.push_back( Column{dbName, value, commonColumns} );
				//DBG( "{}.{}, dt={}"sv, name, columnName, Columns.back().DataTypeString() );
				if( auto p=find(SurrogateKey.begin(), SurrogateKey.end(), dbName); p!=SurrogateKey.end() )
				{
					Columns.back().IsId = true;
					Columns.back().Updateable = false;
				}
			}
		}
	}

	string Table::InsertProcName()const noexcept
	{
		var haveSequence = std::find_if( Columns.begin(), Columns.end(), [](var& c){return c.IsIdentity;} )!=Columns.end();
		return !haveSequence /*|| FlagsData.size() || Data.size()*/ ? string{} : format( "{}_insert", DB::Schema::ToSingular(Name) );
	}

	string Table::InsertProcText( const Syntax& syntax )const noexcept
	{
		ostringstream osCreate, osInsert, osValues;
		//if( syntax.AltDelimiter().size() )
		//	osCreate << "delimiter " << syntax.AltDelimiter() << endl;
		osCreate << "create procedure " << InsertProcName() << "(";
		osInsert << "\tinsert into " << Name << "(";
		osValues << "\t\tvalues(";
		var prefix = syntax.ProcParameterPrefix().empty() ? "_"sv : syntax.ProcParameterPrefix();
		char delimiter = ' ';
		//string sequenceType;
		for( var& column : Columns )
		{
			//if( column.IsIdentity )
			//	sequenceType = column.DataTypeString();
			string value = format( "{}{}", prefix, column.Name );
			if( column.Insertable )
				osCreate << delimiter << prefix << column.Name  << " " << column.DataTypeString();
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
		osCreate << " )" << endl << "begin" << endl;
		//osCreate << "declare _id " << sequenceType << ";" << endl;
		osCreate << osInsert.str() << osValues.str();
		osCreate << "\tselect LAST_INSERT_ID();" << endl << "end" << endl;// into _id
		//if( syntax.AltDelimiter().size() )
		//	osCreate << "delimiter ;";
		return osCreate.str();
	}

	string Table::Create( const Syntax& syntax )const noexcept
	{
		ostringstream createStatement;
		createStatement << "Create table " << Name << "(" << endl;
		string suffix = "";
		string pk;
		for( var& column : Columns )
		{
			if( column.IsIdentity )
				pk = format( "PRIMARY KEY({})", column.Name );
			createStatement << suffix << endl << "\t" << column.Create( syntax );
			suffix = ",";
		}
		if( pk.size() )
			createStatement << suffix << endl << "\t" << pk << endl;
		createStatement << ")";
		return createStatement.str();
	}

/*	Index::Index( sv indexName, sv tableName, bool primaryKey, bool unique, bool clustered )noexcept:
		//TablePtr{pTable},
		Name{indexName},
		TableName{ tableName },
		Clustered{clustered},
		Unique{unique},
		PrimaryKey{primaryKey}
	{}
	*/
	Index::Index( sv indexName, sv tableName, bool primaryKey, vector<string>* pColumns, bool unique, optional<bool> clustered )noexcept:
		Name{ indexName },
		TableName{ tableName },
		Columns{ pColumns ? *pColumns : vector<string>{} },
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

	string Index::Create( sv name, sv tableName, const Syntax& syntax )const noexcept
	{
		string unique = Unique ? "unique" : "";
		ostringstream os;
		if( PrimaryKey )
			os << "alter table " << tableName << " add constraint " << name << (Clustered || !syntax.SpecifyIndexCluster() ? "" : " nonclustered") << " primary key(";
		else
			os << "create " << (Clustered && syntax.SpecifyIndexCluster() ? "clustered " : " ") << unique << " index "<< name << " on " << endl << tableName << "(";

		os << StringUtilities::AddCommas( Columns ) << ")";

		return os.str();
	}

	string ForeignKey::Create( sv name, sv columnName, const DB::Table& pkTable, sv foreignTable )noexcept(false)
	{
		//vector<Column> surrogateKeys;
		//std::for_each( pkTable.Columns.begin(), pkTable.Columns.end(), [&](auto& x){if( x.IsId ) surrogateKeys.push_back(x);} );
		THROW_IF( pkTable.SurrogateKey.size()!=1, EnvironmentException("{} has {} columns in pk, !1 has not implemented", pkTable.Name, pkTable.SurrogateKey.size() ) );
		ostringstream os;
		os << "alter table " << foreignTable << " add constraint " << name << " foreign key(" << columnName << ") references " << pkTable.Name << "(" << StringUtilities::AddCommas(pkTable.SurrogateKey) << ")";
		return os.str();
	}
	const Column* Table::FindColumn( sv name )const noexcept
	{
		auto pColumn = find_if( Columns.begin(), Columns.end(), [&name](var& c){return c.Name==name;} );
		return pColumn==Columns.end() ? nullptr : &(*pColumn);
	}

	string ColumnStartingWith( const Table& table, sv part )
	{
		auto pColumn = find_if( table.Columns.begin(), table.Columns.end(), [&part](var& c){return c.Name.starts_with(part);} );
		return pColumn==table.Columns.end() ? string{} : pColumn->Name;
	}

	string TableNamePart( const Table& table, uint8 index )noexcept(false)
	{
		var nameParts = StringUtilities::Split( table.NameWithoutType(), '_' ); //THROW_IF( nameParts.size()!=3, Exception("Child/Parent expected 3 parts to table name {}", table.Name) );
		return nameParts.size()>index ? DB::Schema::ToSingular( nameParts[index] ) : string{};
	}
	string Table::Prefix()const noexcept{ return TableNamePart(*this, 0); }
	string Table::NameWithoutType()const noexcept{ var underscore = Name.find_first_of('_'); return Name.substr(underscore==string::npos ? 0 : underscore+1); }

	string Table::FKName()const noexcept{ return Schema::ToSingular(NameWithoutType())+"_id"; }
	string Table::JsonTypeName()const noexcept
	{
		auto name = Schema::ToJson( Schema::ToSingular(NameWithoutType()) );
		if( name.size() )
			name[0] = std::toupper( name[0] );
		return name;
	}
	string Table::ChildId()const noexcept(false)
	{
		var part = TableNamePart( *this, 0 );
		return part.empty() ? part : ColumnStartingWith( *this, part );
	}

	sp<const Table> Table::ChildTable( const DB::Schema& schema )const noexcept(false)
	{
		var part = TableNamePart( *this, 0 );
		return part.empty() ? sp<const Table>{} : schema.FindTableSuffix( Schema::ToPlural(part) );
	}

	string Table::ParentId()const noexcept(false)
	{
		var part = TableNamePart( *this, 1 );
		return part.empty() ? part : ColumnStartingWith( *this, part );
	}

	sp<const Table> Table::ParentTable( const DB::Schema& schema )const noexcept(false)
	{
		var part = TableNamePart( *this, 1 );
		return part.empty() ? sp<const Table>{} : schema.FindTableSuffix( Schema::ToPlural(part) );
	}

}