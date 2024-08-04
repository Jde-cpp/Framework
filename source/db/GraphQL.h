#pragma once
#ifndef GRAPHQL_H
#define GRAPHQL_H
#include <variant>
#include <jde/Str.h>
#include <jde/db/graphQL/TableQL.h>
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
	Φ CoQuery( string query, UserPK userId, str threadName, SRCE )ι->Coroutine::TPoolAwait<json>;

	typedef std::variant<vector<TableQL>,MutationQL> RequestQL;

	α ParseQL( sv query )ε->RequestQL;

	Φ AddMutationListener( string tablePrefix, function<void(const DB::MutationQL& m, PK id)> listener )ι->void;
}
#undef var
#undef Φ
#endif