#include "gtest/gtest.h"
#include "../source/um/UM.h"
#include "../source/db/GraphQL.h"
#include <jde/Log.h>

#define var const auto
namespace Jde::DB
{
	using nlohmann::json;
	static const LogTag& _logLevel = Logging::TagLevel( "tests" );

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
		var defTest = format( "{{ __type(name: \"{}\") {{ fields {{ name type {{ name kind ofType{{name kind}} }} }} }} }}", "RolePermission" );
		var defTestItems = DB::Query( defTest, 0 );
		var result = defTestItems.dump();
		Dbg( result );
		var expected = "{\"data\":{\"__type\":{\"fields\":[{\"name\":\"id\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":0,\"name\":\"ID\"}}},{\"name\":\"api\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":4,\"name\":\"Api\"}}},{\"name\":\"name\",\"type\":{\"kind\":0,\"name\":\"String\"}},{\"name\":\"rights\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":6,\"name\":\"Right\"}}}],\"name\":\"RolePermission\"}}}"sv;
		ASSERT_EQ( result, expected );
	}

	TEST_F( QLTests, IntrospectionDefTests )
	{
		//var defTest = format("{{ __type(name: \"{}\") {{ fields {{ name type {{ name kind ofType{{name kind}} }} }} }} }}"

		var defTest = format("{{ __type(name: \"{}\") {{ fields {{ name type {{ name kind ofType{{name kind}} }} }} }} }}", "RolePermission" );
		var defTestItems = DB::Query( defTest, 0 );
		Dbg( defTestItems.dump() );
		ASSERT_EQ( defTestItems.dump(), "{\"data\":{\"__type\":{\"fields\":[{\"name\":\"id\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":0,\"name\":\"ID\"}}},{\"name\":\"api\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":4,\"name\":\"Api\"}}},{\"name\":\"name\",\"type\":{\"kind\":0,\"name\":\"String\"}},{\"name\":\"rights\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":6,\"name\":\"Right\"}}}],\"name\":\"RolePermission\"}}}" );
	}

	TEST_F( QLTests, Filter)
	{
		var ql = "query{ rights(filter: {id: {ne: 0}}){ id name } }"sv;
		var items = DB::Query( ql, 0 );
		Dbg( items.dump() );
		ASSERT_EQ( items.dump(), "{\"data\":{\"rights\":[{\"id\":1,\"name\":\"Administer\"},{\"id\":4,\"name\":\"Read\"},{\"id\":2,\"name\":\"Write\"}]}}" );
	}

	void SetAttribute( const json& source, json& dest, sv attribute )
	{
		for( var& [name,value] : source.items() )
		{
			if( value.is_object() )
				SetAttribute( value, *dest.find(name), attribute );
			else if( auto p = dest.find(name); p!=dest.end() )
			{
				if( value.is_array() )
				{
					for( uint i=0; i<value.size(); ++i )
					{
						if( value[i].is_object() )
							SetAttribute( value[i], p->at(i), attribute );
					}
				}
				else if( name==attribute )
					*dest.find( name ) = value;
			}
			else
			{
				//DBG( "{}={}", name, value );
				dest[name] = value;
			}

		}
/*		for( var& [name,value] : dest.items() )//not in source, but in destination
		{
			DBG( "{}={}", name, value );
			if( name==attribute )
				DBG( "{}", value );
			if( value.is_object() )
				SetAttribute( value, *dest.find(name), attribute );
			else if( auto p = source.find(name); p==source.end() && name==attribute )
				dest.erase( dest.find(name) );
		}*/
	}
	α CreateUser()
	{
		//{ mutation { createUser(  "input": {"name":"TraderX2@gmail.com","target":"TraderX2@gmail.com","description":"TraderX2@gmail.com","authenticator":1} ){id} } }
		constexpr sv ql = "{mutation {createUser( \"input\": {\"name\":\"JohnSmith@google.com\",\"target\":\"jsmith\",\"description\":\"Unit Test User\",\"authenticator\":1} ){id}} }";
		var db = DB::Query( ql, 0 );
		Dbg( db.dump() );
		return db.find( "data" )->find( "user" )->find( "id" )->get<uint>();
	}
	constexpr uint UserId = 0;
	α FetchUser()
	{
		constexpr sv ql = "query{ user(name:\"JohnSmith@google.com\") { id name attributes created deleted updated description target authenticator } }";
		return DB::Query( ql, UserId );
	}
	TEST_F( QLTests, CreateUser )
	{
		auto results = FetchUser();
		LOGS( results.dump() );
		if( var p=results.find("data")->find("user"); p!=results.find("data")->end() )
		{
			uint id = p->find( "id" )->get<uint>();
			DB::Query( format("{{ mutation {{ deleteUser(\"id\":{}) }} }}", id), UserId );
			DB::Query( format("{{ mutation {{ purgeUser(\"id\":{}) }} }}", id), UserId );
		}
		CreateUser();
		results = FetchUser();
		LOGS( results.dump() );
		auto expected = "{\"data\":{\"user\":{\"created\":\"2021-10-21T09:58:59Z\",\"description\":\"Unit Test User\",\"id\":1003,\"name\":\"JohnSmith@google.com\",\"target\":\"jsmith\", \"authenticator\": \"Google\"}}}"_json;
		SetAttribute( results, expected, "created" );
		SetAttribute( results, expected, "id" );
		ASSERT_EQ( results.dump(), expected.dump() );

	}
	TEST_F( QLTests, DefTestsFetch)
	{
		var ql = "query{ role(target:\"user_management\"){ id name created deleted updated description target groups{id name created deleted updated description target } rolePermissions{api{ id name } id name rights } } }"sv;
		var db = DB::Query( ql, 0 );
		//Dbg( db.dump() );

		auto expected = "{\"data\":{\"role\":{\"created\":\"2021-02-13T07:00:33Z\",\"groups\":[{\"created\":\"2021-02-13T07:00:33Z\",\"id\":1,\"name\":\"Everyone\",\"target\":\"everyone\"},{\"created\":\"2021-02-13T07:00:33Z\",\"id\":2,\"name\":\"Users\",\"target\":\"users\"}],\"id\":1,\"name\":\"User Management\",\"rolePermissions\":[{\"api\":{\"id\":1,\"name\":\"UM\"},\"id\":1,\"rights\":[\"Administer\",\"Write\",\"Read\"]}],\"target\":\"user_management\"}}}"_json;
		SetAttribute( db, expected, "created" );
		SetAttribute( db, expected, "description" );
		SetAttribute( db, expected, "id" );
		var expected2 = expected.dump(); var actual = db.dump();
		ASSERT_EQ( actual, expected2 );
	}

	TEST_F( QLTests, ObjectFetch)
	{
		var ql = "query{ permissions(filter: null) {api{name} id name} }"sv;
		var actual = DB::Query( ql, 0 ).dump();
		Dbg( actual );
		auto expected = "{\"data\":{\"permissions\":[{\"api\":{\"name\":\"UM\"},\"id\":1},{\"api\":{\"name\":\"Tws\"},\"id\":2}]}}";
		ASSERT_EQ( actual, expected );
	}


	TEST_F( QLTests, EnumFetchApi)
	{
		var ql = "query{ __type(name: \"Api\") { enumValues { name description } } }"sv;
		var items = DB::Query( ql, 0 );
		Dbg( items.dump() );
		ASSERT_EQ( items.dump(), "{\"data\":{\"__type\":{\"enumValues\":[{\"name\":\"None\"},{\"name\":\"UM\"},{\"name\":\"Web\"},{\"name\":\"Tws\"},{\"name\":\"Blockly\"}]}}}" );
	}

	TEST_F( QLTests, EnumFetchAuthenticator)
	{
		var ql = "query{ __type(name: \"Authenticator\") { enumValues { id name } } }"sv;
		var items = DB::Query( ql, 0 );
		Dbg( items.dump() );
		ASSERT_EQ( items.dump(), "{\"data\":{\"__type\":{\"enumValues\":[{\"id\":0,\"name\":\"None\"},{\"id\":1,\"name\":\"Google\"}]}}}" );
	}

	TEST_F( QLTests, Introspection)
	{
		var ql = "query{ role(id:1){ groups{id name attributes created deleted updated description target} } }";
		//var ql = " { mutation{ removeGroupRole(\"input\":{ \"roleId\": 1, \"groupId\": 1} ) } }";
		//var ql = "query{ role(target:\"user_management\"){ id name attributes created deleted updated description target  groups{id name attributes created deleted updated description target } permissions{api{ id name } id name } } }";
		var items = DB::Query( ql, 0 );
		Dbg( items.dump() );
	}
	TEST_F( QLTests, IntrospectionSchema)
	{
		var ql = "query{__schema{mutationType{name fields { name args { name defaultValue type { name } } } } }";
		var items = DB::Query( ql, 0 );
		//var x = items.dump();
		//Dbg( x );
		ASSERT_EQ( 0, 0 );
	}
}
