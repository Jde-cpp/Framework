namespace Jde::DB
{
#define $ const noexcept
	struct Syntax
	{
		virtual ~Syntax()=default;

		β AddDefault( sv tableName, sv columnName, sv columnDefault )$->string{ return format("alter table {} add default {} for {}", tableName, columnDefault, columnName); }
		β AltDelimiter()$->sv{ return {}; }
		β CatalogSelect()$->sv{ return "select db_name();"; }
		β DateTimeSelect( sv columnName )$->string{ return string{columnName}; }
		β HasUnsigned()$->bool{ return false; }
		β IdentityColumnSyntax()$->sv{ return "identity(1001,1)"sv; }
		β IdentitySelect()$->sv{ return "@@identity"sv; }
		β Limit( str syntax, uint limit )const noexcept(false)->string;
		β NowDefault()$->sv{ return UtcNow(); }
		β ProcFileSuffix()$->sv{ return ".ms"; }
		β ProcParameterPrefix()$->sv{ return "@"sv; }
		β ProcStart()$->sv{ return "as\n\tset nocount on;\n"sv; }
		β ProcEnd()$->sv{ return ""sv; }
		β SpecifyIndexCluster()$->bool{ return true; }
		β UniqueIndexNames()$->bool{ return false; }
		β UtcNow()$->sv{ return "getutcdate()"sv; }
		β ZeroSequenceMode()$->sv{ return {}; }
	};

	struct MySqlSyntax final: Syntax
	{
		α AddDefault( sv tableName, sv columnName, sv columnDefault )$->string override{ return format("ALTER TABLE {} ALTER COLUMN {} SET DEFAULT {}", tableName, columnName, columnDefault); }
		α AltDelimiter()$->sv override{ return "$$"sv; }
		α DateTimeSelect( sv columnName )$->string override{ return format( "UNIX_TIMESTAMP(CONVERT_TZ({}, '+00:00', @@session.time_zone))", columnName ); }
		α HasUnsigned()$->bool override{ return true; }
		α IdentityColumnSyntax()$->sv override{ return "AUTO_INCREMENT"sv; }
		α IdentitySelect()$->sv override{ return "LAST_INSERT_ID()"sv; }
		β Limit( str sql, uint limit )$->string override{ return format("{} limit {}", sql, limit); }
		α ProcEnd()$->sv override{ return "end"sv; }
		α ProcParameterPrefix()$->sv override{ return ""sv; }
		α ProcStart()$->sv override{ return "begin"sv; }
		α SpecifyIndexCluster()$->bool override{ return false; }
		α UtcNow()$->sv override{ return "CURRENT_TIMESTAMP()"sv; }
		α NowDefault()$->sv override{ return "CURRENT_TIMESTAMP"sv; }
		α ZeroSequenceMode()$->sv override{ return "SET @@session.sql_mode = CASE WHEN @@session.sql_mode NOT LIKE '%NO_AUTO_VALUE_ON_ZERO%' THEN CASE WHEN LENGTH(@@session.sql_mode)>0 THEN CONCAT_WS(',',@@session.sql_mode,'NO_AUTO_VALUE_ON_ZERO') ELSE 'NO_AUTO_VALUE_ON_ZERO' END ELSE @@session.sql_mode END"sv; }
		α CatalogSelect()$->sv override{ return "select database() from dual;"; }
		α ProcFileSuffix()$->sv override{ return ""; }
	};

	Ξ Syntax::Limit( str sql, uint limit )const noexcept(false)->string
	{
		if( sql.size()<7 )//mysql precludes using THROW, THROW_IF, CHECK
			throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Debug, "expecting sql length>7 - {}", sql };
		return format("{} top {} {}", sql.substr(0,7), limit, sql.substr(7) );
	};
}
#undef $