#include <variant>
#include <nlohmann/json.hpp>
#include <jde/Str.h>
#include "../coroutine/Awaitable.h"
#include "../um/UM.h"

#define var const auto
#define Φ Γ auto
namespace Jde::DB
{
	struct Schema; struct IDataSource; struct Syntax; struct Column;
	namespace GraphQL
	{
		α DataSource()ι->sp<IDataSource>;
	}
	α AppendQLSchema( const Schema& schema )ι->void;
	α SetQLDataSource( sp<IDataSource> p )ι->void;
	α ClearQLDataSource()ι->void;
	enum class QLFieldKind : uint8{ Scalar=0, Object=1, Interface=2, Union=3, Enum=4, InputObject=5, List=6, NonNull=7 };
	constexpr array<sv,8> QLFieldKindStrings = { "SCALAR", "OBJECT", "INTERFACE", "UNION", "ENUM", "INPUT_OBJECT", "LIST", "NON_NULL" };

	Φ Query( sv query, UserPK userId )ε->nlohmann::json;
	Φ CoQuery( string query, UserPK userId, SRCE )ι->Coroutine::PoolAwait;
	struct ColumnQL final
	{
		string JsonName;
		mutable const Column* SchemaColumnPtr{nullptr};
		Ω QLType( const DB::Column& db, SRCE )ε->string;
	};

	struct TableQL final
	{
		α DBName()Ι->string;
		const ColumnQL* FindColumn( sv jsonName )Ι{ auto p = find_if( Columns.begin(), Columns.end(), [&](var& c){return c.JsonName==jsonName;}); return p==Columns.end() ? nullptr : &*p; }
		sp<const TableQL> FindTable( sv jsonTableName )Ι{ auto p = find_if( Tables.begin(), Tables.end(), [&](var t){return t->JsonName==jsonTableName;}); return p==Tables.end() ? sp<const TableQL>{} : *p; }
		string JsonName;
		nlohmann::json Args;
		vector<ColumnQL> Columns;
		vector<sp<const TableQL>> Tables;
	};
	enum class EMutationQL : uint8{ Create=0, Update=1, Delete=2, Restore=3, Purge=4, Add=5, Remove=6 };
	constexpr array<sv,7> MutationQLStrings = { "create", "update", "delete", "restore", "purge", "add", "remove" };
	struct MutationQL final
	{
		MutationQL( sv json, EMutationQL type, const nlohmann::json& args, optional<TableQL> resultPtr/*, sv parent*/ ):JsonName{json}, Type{type}, Args(args), ResultPtr{resultPtr}/*, ParentJsonName{parent}*/{}
		α TableSuffix()Ι->string;
		string JsonName;
		EMutationQL Type;
		nlohmann::json Args;
		optional<TableQL> ResultPtr;
		nlohmann::json Input()Ε;
		nlohmann::json InputParam( sv name )Ε;
		PK ParentPK()Ε;
		PK ChildPK()Ε;
	private:
		mutable string _tableSuffix;
	};
	typedef std::variant<vector<TableQL>,MutationQL> RequestQL;

	RequestQL ParseQL( sv query )ε;

	Φ AddMutationListener( string tablePrefix, function<void(const DB::MutationQL& m, PK id)> listener )ι->void;
}
#undef var
#undef Φ