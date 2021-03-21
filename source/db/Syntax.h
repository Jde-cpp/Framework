
namespace Jde::DB
{
	struct Syntax
	{
		virtual ~Syntax()=default;
		virtual bool UniqueIndexNames()const noexcept{ return false; }
		virtual bool SpecifyIndexCluster()const noexcept{ return true; }
		virtual sv AltDelimiter()const noexcept{ return {}; }
		virtual sv ProcParameterPrefix()const noexcept{ return "@"sv; }
		virtual sv IdentityColumnSyntax()const noexcept{ return "identity"sv; }
		virtual sv UtcNow()const noexcept{ return "utcnow()"sv; }
		virtual string DateTimeSelect( sv columnName )const noexcept{ return string{columnName}; }
		virtual sv ZeroSequenceMode()const noexcept{ return {}; }
	};
	struct MySqlSyntax final: Syntax
	{
		bool SpecifyIndexCluster()const noexcept{ return false; }
		sv AltDelimiter()const noexcept{ return "$$"sv; }
		sv ProcParameterPrefix()const noexcept{ return ""sv; }
		sv IdentityColumnSyntax()const noexcept{ return "AUTO_INCREMENT"sv; }
		sv UtcNow()const noexcept{ return "CURRENT_TIMESTAMP()"sv; }
		string DateTimeSelect( sv columnName )const noexcept{ return format( "UNIX_TIMESTAMP(CONVERT_TZ({}, '+00:00', @@session.time_zone))", columnName ); }
		sv ZeroSequenceMode()const noexcept{ return "SET @@session.sql_mode = CASE WHEN @@session.sql_mode NOT LIKE '%NO_AUTO_VALUE_ON_ZERO%' THEN CASE WHEN LENGTH(@@session.sql_mode)>0 THEN CONCAT_WS(',',@@session.sql_mode,'NO_AUTO_VALUE_ON_ZERO') ELSE 'NO_AUTO_VALUE_ON_ZERO' END ELSE @@session.sql_mode END"sv; }
	};
}