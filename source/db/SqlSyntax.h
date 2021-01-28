

namespace Jde::DB
{
	struct SqlSyntax
	{
		virtual ~SqlSyntax()=default;
		virtual bool UniqueIndexNames()const noexcept{ return false; }
		virtual bool SpecifyIndexCluster()const noexcept{ return false; } //true for mssql.
		virtual sv AltDelimiter()const noexcept{ return "$$"sv; } //{} for mssql.
		virtual sv ProcParameterPrefix()const noexcept{ return ""sv; }//@ for mssql.
		virtual sv IdentityColumnSyntax()const noexcept{ return "AUTO_INCREMENT"sv; }//identity? for mssql.
		virtual sv UtcNow()const noexcept{ return "CURRENT_TIMESTAMP()"sv; }//
		virtual string DateTimeSelect( sv columnName )const noexcept{ return format( "UNIX_TIMESTAMP(CONVERT_TZ({}, '+00:00', @@session.time_zone))", columnName ); }//
		virtual sv ZeroSequenceMode()const noexcept{ return "SET @@session.sql_mode = CASE WHEN @@session.sql_mode NOT LIKE '%NO_AUTO_VALUE_ON_ZERO%' THEN CASE WHEN LENGTH(@@session.sql_mode)>0 THEN CONCAT_WS(',',@@session.sql_mode,'NO_AUTO_VALUE_ON_ZERO') ELSE 'NO_AUTO_VALUE_ON_ZERO' END ELSE @@session.sql_mode END"sv; }
	};
}