
namespace Jde::DB
{
	struct Syntax
	{
		virtual ~Syntax()=default;

		virtual string AddDefault( sv tableName, sv columnName, sv columnDefault )const noexcept{ return format("alter table {} add default {} for {}", tableName, columnDefault, columnName); }
		virtual sv AltDelimiter()const noexcept{ return {}; }
		virtual string DateTimeSelect( sv columnName )const noexcept{ return string{columnName}; }
		virtual bool HasUnsigned()const noexcept{ return false; }
		virtual sv IdentityColumnSyntax()const noexcept{ return "identity(1001,1)"sv; }
		virtual sv IdentitySelect()const noexcept{ return "@@identity"sv; }
		virtual sv ProcStart()const noexcept{ return "as\n\tset nocount on;\n"sv; }
		virtual sv ProcParameterPrefix()const noexcept{ return "@"sv; }
		virtual sv ProcEnd()const noexcept{ return ""sv; }
		virtual bool SpecifyIndexCluster()const noexcept{ return true; }
		virtual bool UniqueIndexNames()const noexcept{ return false; }
		virtual sv UtcNow()const noexcept{ return "getutcdate()"sv; }
		virtual sv NowDefault()const noexcept{ return UtcNow(); }
		virtual sv ZeroSequenceMode()const noexcept{ return {}; }
		virtual sv CatalogSelect()const noexcept{ return "select db_name();"; }
		virtual sv ProcFileSuffix()const noexcept{ return ".ms"; }
	};

	struct MySqlSyntax final: Syntax
	{
		string AddDefault( sv tableName, sv columnName, sv columnDefault )const noexcept override{ return format("ALTER TABLE {} ALTER COLUMN {} SET DEFAULT {}", tableName, columnName, columnDefault); }
		sv AltDelimiter()const noexcept override{ return "$$"sv; }
		string DateTimeSelect( sv columnName )const noexcept override{ return format( "UNIX_TIMESTAMP(CONVERT_TZ({}, '+00:00', @@session.time_zone))", columnName ); }
		virtual bool HasUnsigned()const noexcept override{ return true; }
		sv IdentityColumnSyntax()const noexcept override{ return "AUTO_INCREMENT"sv; }
		sv IdentitySelect()const noexcept override{ return "LAST_INSERT_ID()"sv; }
		sv ProcEnd()const noexcept override{ return "end"sv; }
		sv ProcParameterPrefix()const noexcept  override{ return ""sv; }
		sv ProcStart()const noexcept override{ return "begin"sv; }
		bool SpecifyIndexCluster()const noexcept override{ return false; }
		sv UtcNow()const noexcept override{ return "CURRENT_TIMESTAMP()"sv; }
		sv NowDefault()const noexcept override{ return "CURRENT_TIMESTAMP"sv; }
		sv ZeroSequenceMode()const noexcept override{ return "SET @@session.sql_mode = CASE WHEN @@session.sql_mode NOT LIKE '%NO_AUTO_VALUE_ON_ZERO%' THEN CASE WHEN LENGTH(@@session.sql_mode)>0 THEN CONCAT_WS(',',@@session.sql_mode,'NO_AUTO_VALUE_ON_ZERO') ELSE 'NO_AUTO_VALUE_ON_ZERO' END ELSE @@session.sql_mode END"sv; }
		sv CatalogSelect()const noexcept override{ return "select database() from dual;"; }
		sv ProcFileSuffix()const noexcept override{ return ""; }
	};
}