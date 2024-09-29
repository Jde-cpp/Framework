#include <jde/db/graphQL/FilterQL.h>
#include <regex>

namespace Jde::DB{
	α FilterQL::Test( const DB::object& value, const vector<FilterValueQL>& filters, ELogTags logTags )ι->bool{
		bool passesFilters{true};
		for( auto p=filters.begin(); passesFilters && p!=filters.end(); ++p )
			passesFilters = p->Test( value, logTags );
		return passesFilters;
	}
	α FilterValueQL::Test( const DB::object& db, ELogTags logTags )Ι->bool{
		bool passesFilters{};
		try{
			switch( Operator ){
			using enum EQLOperator;
			case Equal:
				passesFilters = Value==ToJson( db );
				break;
			case EQLOperator::NotEqual:
				passesFilters = Value!=ToJson(db);
				break;
			case EQLOperator::Regex:
				passesFilters = std::regex_match( get<string>(db), std::regex{Value.get<string>()} );
				break;
			case EQLOperator::Glob:
				passesFilters = std::regex_match( get<string>(db), std::regex{Value.get<string>()} );//TODO test this
				break;
			case EQLOperator::In:
				passesFilters = Value.find( ToJson(db) )!=Value.end();
				break;
			case EQLOperator::NotIn:
				passesFilters = Value.find( ToJson(db) )==Value.end();
				break;
			case EQLOperator::GreaterThan:
				passesFilters = Value > ToJson( db );
				break;
			case EQLOperator::GreaterThanOrEqual:
				passesFilters = Value >= ToJson( db );
				break;
			case EQLOperator::LessThan:
				passesFilters = Value < ToJson( db );
				break;
			case EQLOperator::LessThanOrEqual:
				passesFilters = Value <= ToJson( db );
				break;
			case EQLOperator::ElementMatch:
				BREAK;//TODO
				//passesFilters = Value==db;
				break;
			}
		}
		catch( const std::exception& e ){
			Debug( logTags | ELogTags::Exception, "FilterValueQL::Test exception={}", e.what() );
		}
		catch( ... ){
			Critical( logTags | ELogTags::Exception, "FilterValueQL::unknown exception" );
		}
		return passesFilters;
	}
}
