#pragma once
#ifndef GRAPHQL_H
#define GRAPHQL_H
#include <variant>
#include <jde/Str.h>
#include "../coroutine/Awaitable.h"
#include "../um/UM.h"

#define var const auto
#define Φ Γ auto
namespace Jde::DB{
	struct Schema; struct IDataSource; struct Syntax; struct Column;
	namespace GraphQL{
		α DataSource()ι->sp<IDataSource>;
		α ToJsonName( DB::Column c )ε->tuple<string,string>;
	}
	α AppendQLDBSchema( const Schema& schema )ι->void;//database tables
	α SetQLIntrospection( json&& j )ε->void;
	Φ SetQLDataSource( sp<IDataSource> p )ι->void;
	α ClearQLDataSource()ι->void;

	Φ Query( sv query, UserPK userId )ε->json;
	Φ CoQuery( string&& query, UserPK userId, str threadName, SRCE )ι->Coroutine::TPoolAwait<json>;
	struct ColumnQL final{
		string JsonName;
		mutable const Column* SchemaColumnPtr{nullptr};
		Ω QLType( const DB::Column& db, SRCE )ε->string;
	};

	struct TableQL final{
		α DBName()Ι->string;
		α FindColumn( sv jsonName )Ι->const ColumnQL*{ auto p = find_if( Columns, [&](var& c){return c.JsonName==jsonName;}); return p==Columns.end() ? nullptr : &*p; }
		α FindTable( sv jsonTableName )Ι->const TableQL*{ auto p = find_if( Tables, [&](var& t){return t.JsonName==jsonTableName;}); return p==Tables.end() ? nullptr : &*p; }
		α Input()Ε->const json&{ auto p =Args.find( "input" ); THROW_IF( p == Args.end(), "Could not find 'input' arg." ); return *p;}
		α IsPlural()Ι{ return JsonName.ends_with( "s" ); }
		string JsonName;
		json Args;
		vector<ColumnQL> Columns;
		vector<TableQL> Tables;
	};
	enum class EMutationQL : uint8{ Create=0, Update=1, Delete=2, Restore=3, Purge=4, Add=5, Remove=6 };
	constexpr array<sv,7> MutationQLStrings = { "create", "update", "delete", "restore", "purge", "add", "remove" };
	struct MutationQL final{
		MutationQL( sv j, EMutationQL type, const json& args, optional<TableQL> resultPtr ):JsonName{j}, Type{type}, Args(args), ResultPtr{resultPtr}{}
		α TableSuffix()Ι->string;
		α Input(SRCE)Ε->json;
		α InputParam( sv name )Ε->json;
		α ParentPK()Ε->PK;
		α ChildPK()Ε->PK;

		string JsonName;
		EMutationQL Type;
		json Args;
		optional<TableQL> ResultPtr;
	private:
		mutable string _tableSuffix;
	};
	typedef std::variant<vector<TableQL>,MutationQL> RequestQL;

	RequestQL ParseQL( sv query )ε;

	Φ AddMutationListener( string tablePrefix, function<void(const DB::MutationQL& m, PK id)> listener )ι->void;
}
#undef var
#undef Φ
#endif