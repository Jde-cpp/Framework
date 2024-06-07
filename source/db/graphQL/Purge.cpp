#include "Purge.h"
#include <jde/db/graphQL/GraphQLHook.h>
#include "../Database.h"
#include "../GraphQL.h"
#include "../GraphQuery.h"

#define var const auto
#define  _schema DB::DefaultSchema()
#define _pDataSource GraphQL::DataSource()

namespace Jde::DB::GraphQL{

	α PurgeStatements( const DB::Table& table, const DB::MutationQL& m, UserPK userPK, vector<DB::object>& parameters, SRCE )ε->vector<string>{
		var pId = m.Args.find( "id" ); THROW_IF( pId==m.Args.end(), "Could not find id argument. {}", m.Args.dump() );
		parameters.push_back( ToObject(EType::ULong, *pId, "id") );
		if( var p=UM::FindAuthorizer(table.Name); p )
			p->TestPurge( *pId, userPK );
		
		sp<const DB::Table> pExtendedFromTable;
		auto pColumn = table.FindColumn( "id" );
		if( !pColumn ){
			if( pExtendedFromTable = table.GetExtendedFromTable(_schema); pExtendedFromTable )
				pColumn = &table.SurrogateKey();
			else
				THROW( "Could not find 'id' column" );
		}
		vector<string> statements{ Jde::format("delete from {} where {}=?", table.Name, pColumn->Name) };
		if( pExtendedFromTable ){
			var extendedPurge = PurgeStatements( *pExtendedFromTable, m, userPK, parameters, sl );
			statements.insert( end(statements), begin(extendedPurge), end(extendedPurge) );
		}
		return statements;
	}

	α PurgeAwait::Execute( const DB::Table& table, DB::MutationQL mutation, UserPK userPK, HCoroutine h )ι->Task{
		vector<DB::object> parameters;
		vector<string> statements;
		try{
			statements = PurgeStatements( table, mutation, userPK, parameters, _sl );
			( co_await Hook::PurgeBefore(mutation, userPK) ).CheckError();
		}
		catch( IException& e ){
			Resume( move(e), move(h) );
			co_return;
		}
		bool success{ true };
		try{
			//TODO for mysql allow CLIENT_MULTI_STATEMENTS return _pDataSource->Execute( Str::AddSeparators(statements, ";"), parameters, sl );
			auto result{ mu<uint>() };
			for( var& statement : statements ){
				auto a = _pDataSource->ExecuteCo(statement, vector<DB::object>{parameters.front()}, _sl); //right now, parameters should singular and the same.
			 	*result += *( co_await *a ).UP<uint>(); //gcc compiler error.
			}
			Resume( move(result), move(h) );
		}
		catch( IException& e ){
			success = false;
		}
		if( !success )
			co_await Hook::PurgeFailure( mutation, userPK );
	}
	PurgeAwait::PurgeAwait( const DB::Table& table, const DB::MutationQL& mutation, UserPK userPK, SL sl )ι:
		AsyncAwait{ [&,user=userPK](HCoroutine h){ Execute( table, mutation, user, move(h) ); }, sl, "PurgeAwait" }
	{}
}