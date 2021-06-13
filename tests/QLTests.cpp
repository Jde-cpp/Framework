#include "gtest/gtest.h"
#include "../source/um/UM.h"
#include "../source/db/GraphQL.h"
//#include "../source/db/types/Schema.h"
#include <jde/Log.h>

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

	TEST_F( QLTests, IntrospectionChildTests )
	{
		var defTest = format( "{{ __type(name: \"{}\") {{ fields {{ name type {{ name kind ofType{{name kind}} }} }} }} }}", "Permission" );
		var defTestItems = DB::Query( defTest, 0 );
		DBG( defTestItems.dump() );
		ASSERT_EQ( defTestItems.dump(), "{\"data\":{\"__type\":{\"fields\":[{\"name\":\"api\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":1,\"name\":\"Api\"}}},{\"name\":\"id\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":0,\"name\":\"ID\"}}},{\"name\":\"name\",\"type\":{\"kind\":0,\"name\":\"String\"}},{\"name\":\"rights\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":6,\"name\":\"Right\"}}}],\"name\":\"RolePermission\"}}}" );
	}

	TEST_F( QLTests, IntrospectionDefTests )
	{
		//var defTest = format("{{ __type(name: \"{}\") {{ fields {{ name type {{ name kind ofType{{name kind}} }} }} }} }}"

		var defTest = format("{{ __type(name: \"{}\") {{ fields {{ name type {{ name kind ofType{{name kind}} }} }} }} }}", "RolePermission" );
		var defTestItems = DB::Query( defTest, 0 );
		DBG( defTestItems.dump() );
		ASSERT_EQ( defTestItems.dump(), "{\"data\":{\"__type\":{\"fields\":[{\"name\":\"api\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":1,\"name\":\"Api\"}}},{\"name\":\"id\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":0,\"name\":\"ID\"}}},{\"name\":\"name\",\"type\":{\"kind\":0,\"name\":\"String\"}},{\"name\":\"rights\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":6,\"name\":\"Right\"}}}],\"name\":\"RolePermission\"}}}" );
	}

	TEST_F(QLTests, Filter)
	{
		var ql = "query{ rights(filter: {id: {ne: 0}}){ id name } }"sv;
		var items = DB::Query( ql, 0 );
		DBG( items.dump() );
		ASSERT_EQ( items.dump(), "{\"data\":{\"rights\":[{\"id\":1,\"name\":\"Administer\"},{\"id\":4,\"name\":\"Read\"},{\"id\":2,\"name\":\"Write\"}]}}" );
	}

	TEST_F(QLTests, DefTestsFetch)
	{
		var ql = "query{ role(target:\"user_management\"){ id name attributes created deleted updated description target  groups{id name attributes created deleted updated description target } rolePermissions{api{ id name } id name rights } } }"sv;
		var items = DB::Query( ql, 0 );
		DBG( items.dump() );
		ASSERT_EQ( items.dump(), "{\"data\":{\"role\":{\"attributes\":4,\"created\":\"2021-02-13T07:00:33Z\",\"groups\":[{\"attributes\":5,\"created\":\"2021-02-13T07:00:33Z\",\"id\":1,\"name\":\"Everyone\",\"target\":\"everyone\"},{\"attributes\":6,\"created\":\"2021-02-13T07:00:33Z\",\"id\":2,\"name\":\"Users\",\"target\":\"users\"}],\"id\":1,\"name\":\"User Management\",\"rolePermissions\":[{\"api\":{\"id\":1,\"name\":\"UM\"},\"id\":1,\"rights\":[\"Administer\",\"Write\",\"Read\"]}],\"target\":\"user_management\"}}}" );
	}

	TEST_F(QLTests, ObjectFetch)
	{
		var ql = "query{ permissions(filter: null) {api{name} id name} }"sv;
		var items = DB::Query( ql, 0 );
		DBG( items.dump() );
		ASSERT_EQ( items.dump(), "{\"data\":{\"permissions\":[{\"api\":{\"name\":\"UM\"},\"id\":1}]}}" );
	}


/*	TEST_F(QLTests, EnumFetch)
	{
		var ql = "query{ __type(name: \"Api\") { enumValues { name description } } }"sv;
		var items = DB::Query( ql, 0 );
		DBG( items.dump() );
		ASSERT_EQ( items.dump(), "{\"data\":{\"__type\":{\"enumValues\":[{\"name\":\"None\"},{\"name\":\"UM\"},{\"name\":\"Web\"},{\"name\":\"Tws\"},{\"name\":\"Blockly\"}]}}}" );
	}
*/
	TEST_F(QLTests, Introspection)
	{
		var ql = "query{ role(id:1){ groups{id name attributes created deleted updated description target} } }";
		//var ql = " { mutation{ removeGroupRole(\"input\":{ \"roleId\": 1, \"groupId\": 1} ) } }";
		//var ql = "query{ role(target:\"user_management\"){ id name attributes created deleted updated description target  groups{id name attributes created deleted updated description target } permissions{api{ id name } id name } } }";
		var items = DB::Query( ql, 0 );
//		DBG( items.dump() );
	}
	TEST_F(QLTests, IntrospectionSchema)
	{
		var ql = "query{__schema{mutationType{name fields { name args { name defaultValue type { name } } } } }";
		var items = DB::Query( ql, 0 );
		DBG( items.dump() );
	}
}
