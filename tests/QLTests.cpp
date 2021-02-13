#include "gtest/gtest.h"
#include "../source/um/UM.h"
#include "../source/db/GraphQL.h"
//#include "../source/db/types/Schema.h"
#include "../source/log/Logging.h"

#define var const auto
namespace Jde::DB
{
	using nlohmann::json;
	struct QLTests : public ::testing::Test
	{
	protected:
		QLTests()noexcept {}
		~QLTests()noexcept override{}

		void SetUp()noexcept override { UM::Configure(); }
		void TearDown()noexcept override {}
	};

	TEST_F(QLTests, Introspection)
	{
		var ql = format("{{ __type(name: \"{}\") {{ fields {{ name type {{ name kind ofType{{name kind}} }} }} }} }}", "Role" );
		//var ql = "query{ roles(deleted:null){name target} }";
		//var ql = "query{ role(target:\"unittestrole1\"){ id name attributes created deleted updated description target groups{id name attributes created deleted updated description target} permissions{apiId id name} } }";
		//var ql = "{ mutation { updateRole( \"id\":1, \"input\": {\"target\":\"user_management2\"} ) } }";
		var items = DB::Query( ql, 0 );
		DBG( items.dump() );
	}
	TEST_F(QLTests, IntrospectionSchema)
	{
		var ql = "query{__schema{mutationType{name fields { name args { name defaultValue type { name } } } } }";
		var items = DB::Query( ql, 0 );
		DBG( items.dump() );
	}
}
