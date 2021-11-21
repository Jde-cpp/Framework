#include <variant>
#include <nlohmann/json.hpp>
#include "../um/UM.h"

#define var const auto
namespace Jde::DB
{
	struct Schema; struct IDataSource; struct Syntax; struct Column;
	namespace GraphQL
	{
		α Schema()noexcept->Schema&;
		α DataSource()noexcept->sp<IDataSource>;
	}
	α AppendQLSchema( const Schema& schema )noexcept->void;
	α SetQLDataSource( sp<IDataSource> p )noexcept->void;
	α ClearQLDataSource()noexcept->void;
	enum class QLFieldKind : uint8
	{
		Scalar=0,
		Object=1,
		Interface=2,
		Union=3,
		Enum=4,
		InputObject=5,
		List=6,
		NonNull=7
	};
	constexpr array<sv,8> QLFieldKindStrings = { "SCALAR", "OBJECT", "INTERFACE", "UNION", "ENUM", "INPUT_OBJECT", "LIST", "NON_NULL" };

	Γ α Query( sv query, UserPK userId )noexcept(false)->nlohmann::json;
	struct ColumnQL final
	{
		string JsonName;
		Column* SchemaColumnPtr{nullptr};
		Ω QLType( const DB::Column& db, SRCE )noexcept(false)->string;
	};

	struct TableQL final
	{
		α DBName()const noexcept->string;
		bool ContainsColumn( sv jsonName )const noexcept{ return find_if( Columns.begin(), Columns.end(), [&](var& c){return c.JsonName==jsonName;})!=Columns.end(); }
		sp<const TableQL> FindTable( sv jsonTableName )const noexcept{ auto p = find_if( Tables.begin(), Tables.end(), [&](var t){return t->JsonName==jsonTableName;}); return p==Tables.end() ? sp<const TableQL>{} : *p; }
		string JsonName;
		nlohmann::json Args;
		vector<ColumnQL> Columns;
		vector<sp<const TableQL>> Tables;
	};
	enum class EMutationQL : uint8
	{
		Create=0,
		Update=1,
		Delete=2,
		Restore=3,
		Purge=4,
		Add=5,
		Remove=6
	};
	constexpr array<sv,7> MutationQLStrings = { "create", "update", "delete", "restore", "purge", "add", "remove" };
	struct MutationQL final
	{
		MutationQL( sv json, EMutationQL type, const nlohmann::json& args, optional<TableQL> resultPtr/*, sv parent*/ ):JsonName{json}, Type{type}, Args(args), ResultPtr{resultPtr}/*, ParentJsonName{parent}*/{}
		α TableSuffix()const noexcept->string;
		string JsonName;
		EMutationQL Type;
		nlohmann::json Args;
		optional<TableQL> ResultPtr;
		nlohmann::json Input()const noexcept(false);
		nlohmann::json InputParam( sv name )const noexcept(false);
		PK ParentPK()const noexcept(false);
		PK ChildPK()const noexcept(false);
	private:
		mutable string _tableSuffix;
	};
	typedef std::variant<vector<TableQL>,MutationQL> RequestQL;

	RequestQL ParseQL( sv query )noexcept(false);

	Γ void AddMutationListener( sv tablePrefix, function<void(const DB::MutationQL& m, PK id)> listener )noexcept;
}
#undef var