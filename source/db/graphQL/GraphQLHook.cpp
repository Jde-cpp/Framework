#include <jde/db/graphQL/GraphQLHook.h>

#pragma GCC diagnostic ignored "-Wswitch"

namespace Jde::DB::GraphQL{
	vector<up<IGraphQLHook>> _hooks; 	mutex _hookMutex;
	α Hook::Add( up<GraphQL::IGraphQLHook>&& hook )ι->void{
		lg _{ _hookMutex };
		_hooks.emplace_back( move(hook) );
	}
	using Hook::Operation;
	α GraphQLHookAwait::CollectAwaits( const DB::MutationQL& mutation, UserPK userPK, Operation op )ι->optional<AwaitResult>{
		up<IAwait> p;
		lg _{ _hookMutex };
		_awaitables.reserve( _hooks.size() );
		for( auto& hook : _hooks ){
			switch( op ){
				case (Operation::Insert | Operation::Before): p = hook->InsertBefore( mutation, userPK ); break;
				case (Operation::Insert | Operation::Failure): p = hook->InsertFailure( mutation, userPK ); break;
				case (Operation::Purge | Operation::Before): p = hook->PurgeBefore( mutation, userPK ); break;
				case (Operation::Purge | Operation::Failure): p = hook->PurgeFailure( mutation, userPK ); break;
			}
			if( p )
				_awaitables.emplace_back( move(p) );
		}
		return _awaitables.empty() ? optional<AwaitResult>{up<uint>{}} : nullopt;
	}

	α GraphQLHookAwait::CollectAwaits( const DB::TableQL& ql, UserPK userPK, Operation op )ι->optional<AwaitResult>{
		up<IAwait> p;
		lg _{ _hookMutex };
		_awaitables.reserve( _hooks.size() );
		for( auto& hook : _hooks ){
			switch( op ){
				case Operation::Select: p = hook->Select( ql, userPK ); break;
			}
			if( p )
				_awaitables.emplace_back( move(p) );
		}
		return _awaitables.empty() ? optional<AwaitResult>{up<json>()} : nullopt;
	}

	α GraphQLHookAwait::AwaitMutation( HCoroutine h )ι->Task{
		for( auto& awaitable : _awaitables ){
			try{
				( co_await *awaitable ).CheckError();
			}
			catch( IException& e ){
				Resume( move(e), h );
				co_return;
			}
		}
		ResumeBool( true, h );
	}

	α GraphQLHookAwait::Await( HCoroutine h )ι->Task{
		up<json> pResult;
		for( auto& awaitable : _awaitables ){
			try{
				pResult = awaitp( json, *awaitable );
			}
			catch( IException& e ){
				Resume( move(e), h );
				co_return;
			}
		}
		Resume( move(pResult), h );
	}

	GraphQLHookAwait::GraphQLHookAwait( const DB::MutationQL& mutation, UserPK userPK_, Operation op_, SL sl )ι:
		AsyncReadyAwait{
			[&, userPK=userPK_, op=op_](){return CollectAwaits(mutation, userPK, op);},
			[&](HCoroutine h){AwaitMutation(h);}, sl, "GraphQLHookAwait" }
	{}

	GraphQLHookAwait::GraphQLHookAwait( const DB::TableQL& ql, UserPK userPK_, Operation op_, SL sl )ι:
		AsyncReadyAwait{ [&, userPK=userPK_, op=op_](){return CollectAwaits(ql, userPK, op);},
			[&](HCoroutine h){Await(h);}, sl, "GraphQLHookAwait" }
	{}

	MutationAwaits::MutationAwaits( sp<DB::MutationQL> mutation, UserPK userPK, Hook::Operation op, SL sl )ι:
		base{ mutation, userPK, sl },
		 _op{op}
	{}

	α MutationAwaits::await_ready()ι->bool{
		lg _{ _hookMutex };
		_awaitables.reserve( _hooks.size() );
		for( auto& hook : _hooks ){
			up<IMutationAwait> p;
			switch( _op ){
				case Operation::Start: p = hook->Start( _mutation, _userPK ); break;
				case Operation::Stop: p = hook->Stop( _mutation, _userPK ); break;
			}
			if( p )
				_awaitables.emplace_back( move(p) );
		}
		return _awaitables.empty();
	}

	α MutationAwaits::await_suspend( base::Handle h )ι->void{
		base::await_suspend( h );
		uint result{};
		for( auto& awaitable : _awaitables ){
			up<IException> pException;
			[&]()->IMutationAwait::Task {
				try{
					result+=co_await *awaitable;
				}
				catch( IException& e ){
					pException = e.Move();
				}
			}();
			if( pException ){
				Promise()->SetError( move(*pException) );
				break;
			}
		}
		if( !Promise()->Error() )
			Promise()->SetValue( move(result) );
		h.resume();
	}

	α Hook::Select( const DB::TableQL& ql, UserPK userPK, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ ql, userPK, Operation::Select, sl }; };
	α Hook::InsertBefore( const DB::MutationQL& mutation, UserPK userPK, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, userPK, Operation::Insert|Operation::Before, sl }; }
	α Hook::InsertFailure( const DB::MutationQL& mutation, UserPK userPK, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, userPK, Operation::Insert|Operation::Failure, sl }; }
	α Hook::PurgeBefore( const DB::MutationQL& mutation, UserPK userPK, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, userPK, Operation::Purge|Operation::Before, sl }; }
	α Hook::PurgeFailure( const DB::MutationQL& mutation, UserPK userPK, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, userPK, Operation::Purge|Operation::Failure, sl }; }
	α Hook::Start( sp<DB::MutationQL> mutation, UserPK userId, SL sl )ι->MutationAwaits{ return MutationAwaits{ mutation, userId, Operation::Start, sl }; }
	α Hook::Stop( sp<DB::MutationQL> mutation, UserPK userId, SL sl )ι->MutationAwaits{; return MutationAwaits{ mutation, userId, Operation::Stop, sl }; }

}