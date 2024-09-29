#include "Table.h"
#include <jde/Str.h>
#include "Schema.h"
#include "../DataType.h"
#include "../Syntax.h"
#include "../../Settings.h"

#define var const auto

namespace Jde::DB
{
	using nlohmann::json;
	using std::endl;

	Column::Column( sv name, uint ordinalPosition, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isIdentity, bool isId, optional<uint> numericPrecision, optional<uint> numericScale )ι:
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

	Column::Column( sv name )ι:
		Name{ name }
	{}

	Column::Column( sv name, const nlohmann::json& j, const flat_map<string,Column>& commonColumns, const flat_map<string,Table>& parents, const nlohmann::ordered_json& schema )ε:
		Name{ name }{
		auto getType = [this,&commonColumns, &schema, &parents]( sv typeName ){
			IsNullable = typeName.ends_with( "?" );
			if( IsNullable )
				typeName = typeName.substr( 0, typeName.size()-1 );
			if( var p = commonColumns.find(string{typeName}); p!=commonColumns.end() ){
				var name = Name; var isNullable = IsNullable;
				*this = p->second;
				Name = name;
				IsNullable = isNullable;
			}
			else{
				Type = ToType( ToIV(typeName) );
				if( Type==EType::None ){
					if( var pPKTable = schema.find(string{typeName}); pPKTable!=schema.end() ){
						Table table{ typeName, *pPKTable, parents, commonColumns, schema };
						if( var pColumn = table.SurrogateKeys.size()==1 ? &table.SurrogateKey() : nullptr; pColumn )
							Type = pColumn->Type;
						IsEnum = table.Data.size();
					}
					if( Type==EType::None )
						Type = EType::UInt;
					PKTable = Schema::FromJson( typeName );
				}
			}
		};
		if( j.is_object() ){
			if( j.contains("sequence") ){
				IsIdentity = j.find( "sequence" )->get<bool>();
				if( IsIdentity )
					IsNullable = Insertable = Updateable = false;
			}
			else{
				if( j.contains("type") )
					getType( j.find("type")->get<string>() );
				if( j.contains("length") ){
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
				if( j.contains("qlAppend") )
					QLAppend = j.find("qlAppend")->get<string>();
				if( j.contains("criteria") )
					Criteria = j.find("criteria")->get<string>();
			}
		}
		else if( j.is_string() )
			getType( j.get<string>() );
	}
	α Column::DataTypeString( const Syntax& syntax )Ι->SchemaName{
		return MaxLength ? Jde::format("{}({})", ToStr(ToString(Type, syntax)), *MaxLength) : ToStr( ToString(Type, syntax) );
	}
	α Column::DefaultObject()Ι->DB::object{
		object obj;
		if( Default.size() ){
			if( Type==EType::Bit )
				obj = Default=="1" || ToIV(Default)=="true" ? 1 : 0;
			else
				obj = Default;
		}
		return obj;
	}

	α Column::Create( const Syntax& syntax )Ι->string{
		var null = IsNullable ? "null"sv : "not null"sv;
		const string sequence = IsIdentity ?  " "+string{syntax.IdentityColumnSyntax()} : string{};
		string dflt;
		if( Default.size() ){
			if( Type==EType::Bit )
				dflt = Jde::format( " default {}", Default=="1" || ToIV(Default)=="true" ? 1 : 0 );
			else
				dflt = Jde::format( " default {}", Default=="$now" ? ToStr(syntax.NowDefault()) : Jde::format("'{}'", Default) );
		}
		return Jde::format( "{} {} {}{}{}", Name, DataTypeString(syntax), null, sequence, dflt );
	}

	Table::Table( sv name, const nlohmann::json& j, const flat_map<string,Table>& parents, const flat_map<string,Column>& commonColumns, const nlohmann::ordered_json& schema ):
		Name{ name }{
		if( var& p = j.find("parent"); p != j.end() ){
			var parentName = p->get<SchemaName>();
			var pParent = parents.find( parentName ); THROW_IF( pParent==parents.end(), "Could not find '{}' parent '{}'", name, parentName );
			for( var& column : pParent->second.Columns )
				Columns.push_back( column );
			for( var& index : pParent->second.Indexes )
				Indexes.push_back( Index{index.Name, Name, index} );
			SurrogateKeys = pParent->second.SurrogateKeys;
		}
		for( var& [attribute,value] : j.items() ){
			if( attribute=="parent" )
				continue;
			if( attribute=="columns" ){
				for( var& it : value ){
					var dbName = Schema::FromJson( it["name"].get<string>() );
					Columns.push_back( Column{dbName, it, commonColumns, parents, schema} );
					if( auto p=find(SurrogateKeys, dbName); p!=SurrogateKeys.end() ){
						Columns.back().IsId = true;
						Columns.back().Updateable = false;
					}
				}
			}
			else if( attribute=="surrogateKey" ){
				for( var& it : value ){
					const string colName{ Schema::FromJson(it.get<string>()) };
					SurrogateKeys.push_back( colName );
					if( auto p=find_if(Columns.begin(), Columns.end(), [&colName](var& x){return x.Name==colName;} ); p!=Columns.end() ){
						p->IsId = true;
						p->Updateable = false;
					}
				}
				Indexes.push_back( Index{"pk", Name, true, &SurrogateKeys} );
			}
			else if( attribute=="naturalKey" ){//TODONext - have multiple in json.  can't have multiple attributes with same name.
				vector<SchemaName> columns;
				for( var& it : value )
					columns.push_back( DB::Schema::FromJson(it.get<string>()) );

				var name2 = NaturalKeys.empty() ? "nk" : Jde::format( "nk{}", NaturalKeys.size() );
				Indexes.push_back( Index{name2, Name, false, &columns} );
				NaturalKeys.push_back( columns );
			}
			else if( attribute=="flagsData" ){
				for( var& it : value )
					FlagsData.emplace( FlagsData.empty() ? 0 : 1 << (FlagsData.size()-1), it );
			}
			else if( attribute=="data" ){
				auto seed = 0;
				for( var& it : value ){
					if( it.is_string() ){
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
			else if( attribute=="purgeProc" )
				PurgeProcName = value.get<string>();
			else if( attribute=="isView" )
				IsView = value.get<bool>();
			else if( attribute=="map"){
				auto parentId = DB::Schema::FromJson( value["parentId"].get<string>() );
				auto pParentColumn = FindColumn( parentId );
				auto childId = DB::Schema::FromJson( value["childId"].get<string>() );
				auto pChildColumn = FindColumn( childId );
				if( pParentColumn && pChildColumn )
					ParentChildMap = make_tuple( ms<Column>(*pParentColumn), ms<Column>(*pChildColumn) );
			}
			else if( attribute=="qlView" )
				QLView = value.get<string>();
			else{
				var _logTag = Settings::LogTag();
				ASSERT_DESC( attribute=="usePrefix" || attribute=="notes", Jde::format("Unknown table attribute:  '{}'", attribute) );
			}
		}
	}

	α Table::InsertProcName()Ι->SchemaName{
		var haveSequence = std::find_if( Columns.begin(), Columns.end(), [](var& c){return c.IsIdentity;} )!=Columns.end();
		return !haveSequence || CustomInsertProc ? SchemaName{} : Jde::format( "{}_insert", DB::Schema::ToSingular(Name) );
	}

	α Table::InsertProcText( const Syntax& syntax )Ι->string{
		ostringstream osCreate, osInsert, osValues;
		osCreate << "create procedure " << InsertProcName() << "(";
		osInsert << "\tinsert into " << Name << "(";
		osValues << "\t\tvalues(";
		var prefix = syntax.ProcParameterPrefix().empty() ? "_"sv : syntax.ProcParameterPrefix();
		char delimiter = ' ';
		for( var& column : Columns ){
			auto value{ Jde::format("{}{}"sv, prefix, column.Name) };
			if( column.Insertable )
				osCreate << delimiter << prefix << column.Name << " " << column.DataTypeString( syntax );
			else{
				if( column.IsNullable || column.Default.empty() )
				 	continue;
				if( column.Default=="$now" )
					value = ToSV( syntax.UtcNow() );
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
		return CIString{ osCreate.str() };
	}

	α Table::GetExtendedFromTable( const DB::Schema& schema )Ι->sp<const Table>{//um_users return um_entities
		auto pPK = SurrogateKeys.size()>0 ? FindColumn( SurrogateKeys[0] ) : nullptr;
		return pPK && pPK->PKTable.size() ? schema.TryFindTable(pPK->PKTable) : sp<const Table>{};
	}

	α Table::Create( const Syntax& syntax )Ι->string{
		ostringstream createStatement;
		createStatement << "Create table " << Name << "(" << endl;
		string suffix = "";
		string pk;
		for( var& column : Columns ){
			if( column.IsIdentity )
				pk = Jde::format( "PRIMARY KEY({})"sv, column.Name );
			createStatement << suffix << endl << "\t" << column.Create( syntax );
			suffix = ",";
		}
		if( pk.size() )
			createStatement << suffix << endl << "\t" << pk << endl;
		createStatement << ")";
		return createStatement.str();
	}

	Index::Index( sv indexName, sv tableName, bool primaryKey, vector<string>* pColumns, bool unique, optional<bool> clustered )ι:
		Name{ indexName },
		TableName{ tableName },
		Columns{ pColumns ? *pColumns : vector<string>{} },
		Clustered{ clustered ? *clustered : primaryKey },
		Unique{ unique },
		PrimaryKey{ primaryKey }
	{}
	Index::Index( sv indexName, sv tableName, const Index& y )ι:
		Name{ indexName },
		TableName{ tableName },
		Columns{ y.Columns },
		Clustered{ y.Clustered },
		Unique{ y.Unique },
		PrimaryKey{ y.PrimaryKey }
	{}

	α Index::Create( sv name, sv tableName, const Syntax& syntax )Ι->string{
		string unique = Unique ? "unique" : "";
		ostringstream os;
		if( PrimaryKey )
			os << "alter table " << tableName << " add constraint " << name << (Clustered || !syntax.SpecifyIndexCluster() ? "" : " nonclustered") << " primary key(";
		else
			os << "create " << (Clustered && syntax.SpecifyIndexCluster() ? "clustered " : " ") << unique << " index "<< name << " on " << endl << tableName << "(";

		os << Str::AddCommas( Columns ) << ")";

		return os.str();
	}

	α ForeignKey::Create( sv name, sv columnName, const DB::Table& pkTable, sv foreignTable )ε->string{
		THROW_IF( pkTable.SurrogateKeys.size()!=1, "{} has {} columns in pk, !1 has not implemented", pkTable.Name, pkTable.SurrogateKeys.size() );
		ostringstream os;
		os << "alter table " << foreignTable << " add constraint " << name << " foreign key(" << columnName << ") references " << pkTable.Name << "(" << Str::AddCommas(pkTable.SurrogateKeys) << ")";
		return os.str();
	}
	α Table::FindColumn( sv name )Ι->const Column*{
		auto pColumn = find_if( Columns.begin(), Columns.end(), [&name](var& c){return c.Name==name;} );
		if( pColumn!=Columns.end() && !pColumn->TablePtr )
			pColumn->TablePtr = this;
		return pColumn==Columns.end() ? nullptr : &(*pColumn);
	}

	α Table::FindColumn( sv name, DB::Schema& schema )Ε->const Column&{
		auto pColumn = FindColumn( name );
		if( var pExtendedFrom = pColumn ? nullptr : GetExtendedFromTable(schema); pExtendedFrom )
			pColumn = pExtendedFrom->FindColumn( name );
		THROW_IF( !pColumn || !pColumn->TablePtr, "Could not find column '{}'", name );
		return *pColumn;
	}

	α ColumnStartingWith( const Table& table, sv part )ι->SchemaName{
		var pColumn = find_if( table.Columns, [&part](var& c){return c.Name.starts_with(part);} );
		return pColumn==table.Columns.end() ? SchemaName{} : pColumn->Name;
	}

	α TableNamePart( const Table& table, uint8 index )ι->string{
		var name = table.NameWithoutType();//split returns temp
		var nameParts = Str::Split( name, '_' );
		return nameParts.size()>index ? DB::Schema::ToSingular( nameParts[index] ) : "";
	}
	α Table::Prefix()Ι->sv{ return Str::Split( Name, '_' )[0]; }
	α Table::NameWithoutType()Ι->sv{ var underscore = Name.find_first_of('_'); return underscore==string::npos ? Name : sv{Name.data()+underscore+1, Name.size()-underscore-1 }; }

	α Table::FKName()Ι->SchemaName{ return string{Schema::ToSingular(NameWithoutType())}+"_id"; }
	α Table::JsonTypeName()Ι->string{
		auto name = Schema::ToJson( Schema::ToSingular(NameWithoutType()) );
		if( name.size() )
			name[0] = (char)std::toupper( name[0] );
		return name;
	}

	α Table::ChildTable( const DB::Schema& schema )Ι->sp<const Table>{
		var pColumn = ChildColumn();
		return pColumn ? schema.TryFindTable( pColumn->PKTable ) : sp<const Table>{};
		// var part = TableNamePart( *this, 0 );
		// return part.empty() ? sp<const Table>{} : schema.TryFindTableSuffix( Schema::ToPlural(part) );
	}

	α Table::ParentTable( const DB::Schema& schema )Ι->sp<const Table>{
		var pColumn = ParentColumn();
		return pColumn ? schema.TryFindTable( pColumn->PKTable ) : sp<const Table>{};
	}
	α Table::SurrogateKey()Ε->const Column&{
		THROW_IF( !IsView && SurrogateKeys.size()==0, "SurrogateKeys.size()={}", SurrogateKeys.size() );//um_groups only wants first.
		var surrogateKey = IsView ? "id" : SurrogateKeys.front();
		auto y = FindColumn( surrogateKey ); THROW_IF( !y, "Could not find SurrogateKey '{}'", surrogateKey );
		return *y;
	}
	α Table::IsEnum()Ι->bool{
		return Columns.size()==2 && Columns[0].Name=="id" && Columns[1].Name=="name";
	}
}