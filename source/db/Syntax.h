namespace Jde::DB
{
	struct Syntax
	{
		virtual ~Syntax()=default;

		β AddDefault( sv tableName, sv columnName, sv columnDefault )Ι->string{ return format("alter table {} add default {} for {}", tableName, columnDefault, columnName); }
		β AltDelimiter()Ι->sv{ return {}; }
		β CatalogSelect()Ι->sv{ return "select db_name();"; }
		β DateTimeSelect( sv columnName )Ι->string{ return string{ columnName }; }
		β HasUnsigned()Ι->bool{ return false; }
		β IdentityColumnSyntax()Ι->sv{ return "identity(1001,1)"sv; }
		β IdentitySelect()Ι->sv{ return "@@identity"sv; }
		β Limit( str syntax, uint limit )const noexcept(false)->string;
		β NowDefault()Ι->iv{ return UtcNow(); }
		β ProcFileSuffix()Ι->sv{ return ".ms"; }
		β ProcParameterPrefix()Ι->sv{ return "@"sv; }
		β ProcStart()Ι->sv{ return "as\n\tset nocount on;\n"sv; }
		β ProcEnd()Ι->sv{ return ""sv; }
		β SpecifyIndexCluster()Ι->bool{ return true; }
		β UniqueIndexNames()Ι->bool{ return false; }
		β UtcNow()Ι->iv{ return "getutcdate()"; }
		β ZeroSequenceMode()Ι->sv{ return {}; }
	};

	struct MySqlSyntax final: Syntax
	{
		α AddDefault( sv tableName, sv columnName, sv columnDefault )Ι->string override{ return format("ALTER TABLE {} ALTER COLUMN {} SET DEFAULT {}", tableName, columnName, columnDefault); }
		α AltDelimiter()Ι->sv override{ return "$$"sv; }
		α DateTimeSelect( sv columnName )Ι->string override{ return format( "UNIX_TIMESTAMP(CONVERT_TZ({}, '+00:00', @@session.time_zone))", columnName ); }
		α HasUnsigned()Ι->bool override{ return true; }
		α IdentityColumnSyntax()Ι->sv override{ return "AUTO_INCREMENT"sv; }
		α IdentitySelect()Ι->sv override{ return "LAST_INSERT_ID()"sv; }
		β Limit( str sql, uint limit )Ι->string override{ return format("{} limit {}", sql, limit); }
		α ProcEnd()Ι->sv override{ return "end"sv; }
		α ProcParameterPrefix()Ι->sv override{ return ""sv; }
		α ProcStart()Ι->sv override{ return "begin"sv; }
		α SpecifyIndexCluster()Ι->bool override{ return false; }
		α UtcNow()Ι->iv override{ return "CURRENT_TIMESTAMP()"; }
		α NowDefault()Ι->iv override{ return "CURRENT_TIMESTAMP"; }
		α ZeroSequenceMode()Ι->sv override{ return "SET @@session.sql_mode = CASE WHEN @@session.sql_mode NOT LIKE '%NO_AUTO_VALUE_ON_ZERO%' THEN CASE WHEN LENGTH(@@session.sql_mode)>0 THEN CONCAT_WS(',',@@session.sql_mode,'NO_AUTO_VALUE_ON_ZERO') ELSE 'NO_AUTO_VALUE_ON_ZERO' END ELSE @@session.sql_mode END"sv; }
		α CatalogSelect()Ι->sv override{ return "select database() from dual;"; }
		α ProcFileSuffix()Ι->sv override{ return ""; }
	};

	Ξ Syntax::Limit( str sql, uint limit )const noexcept(false)->string
	{
		if( sql.size()<7 )//mysql precludes using THROW, THROW_IF, CHECK
			throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Debug, "expecting sql length>7 - {}", sql };
		return format("{} top {} {}", sql.substr(0,7), limit, sql.substr(7) );
	};
}