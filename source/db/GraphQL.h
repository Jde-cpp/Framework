#include <variant>
#include "../um/UM.h"

namespace Jde::DB
{
	struct Schema; struct IDataSource; struct SqlSyntax;
	void AppendQLSchema( const DB::Schema& schema )noexcept;
	void SetQLDataSource( sp<DB::IDataSource> p, sp<DB::SqlSyntax> pSyntax )noexcept;
	void ClearQLDataSource()noexcept;

	nlohmann::json Query( sv query, UserPK userId )noexcept(false);
	struct ColumnQL final
	{
		string JsonName;
	};

	struct TableQL final
	{
		string DBName()const noexcept;
		string JsonName;
		nlohmann::json Args;
		vector<ColumnQL> Columns;
		vector<TableQL> Tables;
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
	constexpr array<string_view,7> MutationQLStrings = { "create", "update", "delete", "restore", "purge", "add", "remove" };
	struct MutationQL final
	{
		MutationQL( sv json, EMutationQL type, nlohmann::json args, optional<TableQL> resultPtr/*, sv parent*/ ):JsonName{json}, Type{type}, Args{args}, ResultPtr{resultPtr}/*, ParentJsonName{parent}*/{}
		string TableSuffix()const noexcept;
		string JsonName;
		EMutationQL Type;
		nlohmann::json Args;
		optional<TableQL> ResultPtr;
		nlohmann::json Input()const noexcept(false);
		nlohmann::json InputParam( sv name )const noexcept(false);
		PK ParentPK()const noexcept(false);
		PK ChildPK()const noexcept(false);
//		string ParentJsonName;
	private:
		mutable string _tableSuffix;
	};
	typedef std::variant<vector<TableQL>,MutationQL> RequestQL;

	RequestQL ParseQL( sv query )noexcept(false);
}