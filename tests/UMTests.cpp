#include "gtest/gtest.h"
#include "../source/um/UM.h"
#include "../source/db/Database.h"
#include "../source/db/GraphQL.h"
#include "../source/db/types/Schema.h"
#include <jde/Log.h>

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
	void AddRemove( sv service, sv childMember, sv parentMember, uint childId, uint parentId, sv suffix={} )
	{
		var frmt = "{{ mutation {{ {}(\"input\":{{ \"{}\": {}, \"{}\": {}{}}} ) }} }}";
		var query = format( frmt, service, childMember, childId, parentMember, parentId, suffix );
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
		//THROW_IF( obj.is_null(), Exception("Could not find '{}' - '{}'- {}", service, name, j.dump()) );
		return obj.is_null() ? 0 : obj["id"].get<uint>();
	}

	TEST_F(UMTests, Users)
	{
		Configure();

		//bool create = true;

		json j1 = DB::Query( "query{ role(filter:{id:{eq:8}}){ rolePermissions{api{ id name } id name rights } } }", 0 );
		DBG( j1.dump() );
		json jx = DB::Query( "query{ authenticators{ id name }, users{ id name target description authenticatorId attributes created updated deleted } }", 0 );
		DBG( jx.dump() );
		auto adminUserId = Query( "user", "JohnSmith@google.com" ); if( !adminUserId ) adminUserId = Crud( "User", "JohnSmith@google.com", "jsmith", "Unit Test User", ",\"authenticatorId\": 1" );
		auto authenticatedUserId = Query( "user", "low-user@google.com" ); if( !authenticatedUserId ) authenticatedUserId = Crud( "User", "low-user@google.com", "low-user", "Unit Test User2", ",\"authenticatorId\": 1" );

		json j = DB::Query( "query{ authenticators{ id name }, users{ id name target description authenticatorId attributes created updated deleted } }", 0 );
		DBG( j.dump() );

		auto administersId = Query( "group", "UnitTestAdminGroup" ); if( !administersId ) administersId = Crud( "Group", "UnitTestAdminGroup", "UnitTestAdminGroup", "UnitTestAdminGroup desc" );
		auto readUMRoleId = Query( "role", "UnitTestRole" ); if( !readUMRoleId ) readUMRoleId = Crud( "Role", "UnitTestRole", "unittest_role_1", "unittest1 role desc" );
		//Give administrator UM rights.
		//Revoke low's write UM Rights.
		//Try to edit.
		//Revoke anonymous' read rights.
		//Try to read.
		//Test administer.
		//Test account.

		var twsApiId = Query( "api", "Tws" );
		sv permissionName = "Act1234"sv;
		auto permissionId = Query( "permission", permissionName );
		if( !permissionId )
		{
			var frmt = "{{ mutation {{ create{}(\"input\":{{ \"name\": \"{}\", \"apiId\": {}}}){{ id }} }} }}";
			var query = format( frmt, "Permission", permissionName, twsApiId );
			var items = DB::Query( query, 0 );
			permissionId = items["data"]["permission"]["id"].get<uint>();
		}

		var updateDef = format( "{{ mutation {{ updateRolePermission(\"roleId\":{}, \"permissionId\":{}, \"input\":{{ \"rights\": [\"Administer\", \"Write\"]}}) }} }}", readUMRoleId, permissionId );
		DB::Query( updateDef, 0 );

		auto pDataSource = Jde::DB::DataSource();
		if( !pDataSource->Scaler<uint>( format("select count(*) from um_group_roles where group_id={} and role_id={}", administersId, readUMRoleId), {}) )
			AddRemove( "addGroupRole", "groupId", "roleId", administersId, readUMRoleId );
		if( !pDataSource->Scaler<uint>( format("select count(*) from um_user_groups where user_id={} and group_id={}", authenticatedUserId, administersId), {}) )
			AddRemove( "addUserGroup", "userId", "groupId", authenticatedUserId, administersId );
		if( !pDataSource->Scaler<uint>( format("select count(*) from um_role_permissions where role_id={} and permission_id={}", readUMRoleId, permissionId), {}) )
			AddRemove( "addRolePermission", "roleId", "permissionId", readUMRoleId, permissionId, format(", \"rightId\": 7") );

		var updateDef2 = format( "{{ mutation {{ updateRolePermission(\"roleId\":{}, \"permissionId\":{}, \"input\":{{ \"rights\": [\"Administer\"]}}) }} }}", 3, 3 );
		DB::Query( updateDef2, 0 );

		var updateDef3 = format( "{{ mutation {{ updateRolePermission(\"roleId\":{}, \"permissionId\":{}, \"input\":{{ \"rights\": null}}) }} }}", 3, 3 );
		DB::Query( updateDef3, 0 );


		AddRemove( "removeGroupRole", "groupId", "roleId", administersId, readUMRoleId );
		AddRemove( "removeUserGroup", "userId", "groupId", authenticatedUserId, administersId );
		AddRemove( "removeRolePermission", "roleId", "permissionId", readUMRoleId, permissionId );

		Purge( "Group", administersId );
		Purge( "User", adminUserId );
		Purge( "User", authenticatedUserId );
		Purge( "Role", readUMRoleId );
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
