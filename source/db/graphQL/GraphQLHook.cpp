#include <jde/db/graphQL/GraphQLHook.h>

#pragma GCC diagnostic ignored "-Wswitch"

namespace Jde::DB::GraphQL{
	mutex _hookMutex;
	vector<up<IGraphQLHook>> _hooks;
	α Hook::Add( up<GraphQL::IGraphQLHook>&& hook )ι->void{
		lg _{ _hookMutex };
		_hooks.emplace_back( move(hook) );
	}
	using Hook::Operation;	
	α GraphQLHookAwait::CollectAwaits( const DB::MutationQL& mutation, Operation op )ι->optional<AwaitResult>{
		up<IAwait> p;
		_awaitables.reserve( _hooks.size() );
		lg _{ _hookMutex };
		for( auto& hook : _hooks ){
			switch( op ){
				case (Operation::Insert | Operation::Before): p = hook->InsertBefore( mutation ); break;
				case (Operation::Insert | Operation::Failure): p = hook->InsertFailure( mutation ); break;
				case (Operation::Purge | Operation::Before): p = hook->PurgeBefore( mutation ); break;
				case (Operation::Purge | Operation::Failure): p = hook->PurgeFailure( mutation ); break;
			}
			if( p )
				_awaitables.emplace_back( move(p) );
		}
		return _awaitables.empty() ? optional<AwaitResult>{true} : nullopt;
	}
	α GraphQLHookAwait::Await( HCoroutine h )ι->Task{
		for( auto& awaitable : _awaitables ){
			try{
				( co_await *awaitable ).CheckError();
			}
			catch( Exception& e ){
				Resume( move(e), move(h) );
				co_return;
			}
		}
		ResumeBool( true, move(h) );
	}

	GraphQLHookAwait::GraphQLHookAwait( const DB::MutationQL& mutation, Operation op, SL sl )ι:
		AsyncReadyAwait{ 
			[&, op2=op](){return CollectAwaits(mutation, op2);}, 
			[&](HCoroutine h){Await(move(h));}, sl, "GraphQLHookAwait" }
	{}

	α Hook::InsertBefore( const DB::MutationQL& mutation, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, Operation::Insert|Operation::Before, sl }; }
	α Hook::InsertFailure( const DB::MutationQL& mutation, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, Operation::Insert|Operation::Failure, sl }; }
	α Hook::PurgeBefore( const DB::MutationQL& mutation, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, Operation::Purge|Operation::Before, sl }; }
	α Hook::PurgeFailure( const DB::MutationQL& mutation, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, Operation::Purge|Operation::Failure, sl }; }
}