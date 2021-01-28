#include "gtest/gtest.h"
#include "../source/um/UM.h"
#include "../source/db/GraphQL.h"
#include "../source/db/types/Schema.h"
#include "../source/log/Logging.h"

#define var const auto
namespace Jde::UM
{
	using nlohmann::json;
	struct UMTests : public ::testing::Test
	{
	protected:
		UMTests()noexcept {}
		~UMTests()noexcept override{}

		void SetUp()noexcept override {}
		void TearDown()noexcept override {}
	};

	TEST_F(UMTests, Schema)
	{
		Configure();
	}

	uint Crud( sv service, sv name, sv target, sv description, sv suffix={} )
	{
		var frmt = "{{ mutation {{ create{}(\"input\":{{ \"name\": \"{}\", \"target\": \"{}\", \"description\": \"{}\"{}}}){{ id }} }} }}";
		var query = format( frmt, service, name, target, description, suffix );
		var items = DB::Query( query, 0 );
		DBG( items.dump() );
		uint id = items["data"][DB::Schema::ToJson(service)]["id"].get<uint>();

		var updateFormat = "{{ mutation {{ update{}(\"id\":{}, \"input\":{{ \"description\": \"{} update\"}}) }} }}";
		var updateQuery = format( updateFormat, service, id, description );
		var updateResult = DB::Query( updateQuery, 0 );

		var deleteFormat = "{{ mutation {{ delete{}(\"id\":{}) }} }}";
		var deleteQuery = format( deleteFormat, service, id );
		DBG( deleteQuery );
		var deleteResult = DB::Query( deleteQuery, 0 );

		return id;
	}
	void Purge( sv service, uint id )
	{
		var purgeFormat = "{{ mutation {{ purge{}(\"id\":{}) }} }}";
		var purgeQuery = format( purgeFormat, service, id );
		//DBG( purgeQuery );
		var purgeResult = DB::Query( purgeQuery, 0 );

		//DBG( purgeResult.dump() );
	}
	void AddRemove( sv service, uint childId, uint parentId, sv suffix={} )
	{
		var frmt = "{{ mutation {{ {}(\"input\":{{ \"childId\": {}, \"parentId\": {}{}}} ) }} }}";
		var query = format( frmt, service, childId, parentId, suffix );
		DB::Query( query, 0 );
	}
	uint Query( sv service, sv name )
	{
		var frmt = "{{query {}(\"name\": \"{}\") {{ id }} }}";
		var query = format( frmt, service, name );
		json j = DB::Query( query, 0 );
		DBG( j.dump() );
		var data = j["data"];
		var obj = j["data"][string{service}];
		THROW_IF( obj.is_null(), Exception("Could not find '{}' - '{}'- {}", service, name, j.dump()) );
		return obj["id"].get<uint>();
	}
	TEST_F(UMTests, Users)
	{
		Configure();

		bool create = true;
		var userId = create ? Crud( "User", "JohnSmith@google.com", "jsmith", "Unit Test User", ",\"authenticatorId\": 1" ) : Query( "user", "JohnSmith@google.com" );
		//DBG( "{}"sv, userId );
//		json j = DB::Query( "{query users{ id name target description authenticatorId attributes created updated deleted } }", 0 );
		json j = DB::Query( "query{ authenticators{ id name }, users{ id name target description authenticatorId attributes created updated deleted } }", 0 );
		DBG( j.dump() );


		var groupId = create ? Crud( "Group", "UnitTest1Group", "unittest1", "unittest1 desc" ) : Query( "group", "UnitTest1Group" );
		var roleId = create ? Crud( "Role", "UnitTestRole", "unittest_role_1", "unittest1 role desc" ) : Query( "role", "UnitTestRole" );
		var twsApiId = Query( "api", "Tws" );
		//var twsApiId = Query( "rights", "Tws" );
		sv permissionName = "Act1234"sv;
		auto permissionId = create ? 0 : Query( "permission", permissionName );
		if( !permissionId )
		{
			var frmt = "{{ mutation {{ create{}(\"input\":{{ \"name\": \"{}\", \"apiId\": {}}}){{ id }} }} }}";
			var query = format( frmt, "Permission", permissionName, twsApiId );
			var items = DB::Query( query, 0 );
			permissionId = items["data"]["permission"]["id"].get<uint>();
		}
		//var permissionId = Crud( "Permission", "UnitTestRole", "unittest_role_1", "unittest1 role desc" );
		//create ? Crud( "Permission", "UnitTestRole", "unittest_role_1", "unittest1 role desc" ) : Query( "permission", "Read" );

		AddRemove( "addGroupRole", groupId, roleId );
		AddRemove( "addUserGroup", userId, groupId );
		AddRemove( "addRolePermission", roleId, permissionId, format(", \"rightId\": 7") );

		AddRemove( "removeGroupRole", groupId, roleId );
		AddRemove( "removeUserGroup", userId, groupId );
		AddRemove( "removeRolePermission", roleId, permissionId );

		Purge( "Group", groupId );
		Purge( "User", userId );
		Purge( "Role", roleId );
		Purge( "Permission", permissionId );

/*		var updateFormat = "{{ mutation {{ updateUser(\"id\":{}, \"input\":{{ \"description\": \"{}\"}}) }} }}";
		var updateQuery = format( updateFormat, id, "Unit Test User Update" );
		//DBG( updateQuery );
		var updateResult = DB::Query( updateQuery, 0 );

		var deleteFormat = "{{ mutation {{ deleteUser(\"id\":{}) }} }}";
		var deleteQuery = format( deleteFormat, id );
		DBG( deleteQuery );
		var deleteResult = DB::Query( deleteQuery, 0 );
*/
	}

}
