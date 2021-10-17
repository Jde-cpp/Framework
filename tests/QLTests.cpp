#include "gtest/gtest.h"
#include "../source/um/UM.h"
#include "../source/db/GraphQL.h"
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
		var defTest = format( "{{ __type(name: \"{}\") {{ fields {{ name type {{ name kind ofType{{name kind}} }} }} }} }}", "RolePermission" );
		var defTestItems = DB::Query( defTest, 0 );
		var result = defTestItems.dump();
		LOG( ELogLevel::Debug, result );
		var expected = "{\"data\":{\"__type\":{\"fields\":[{\"name\":\"api\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":4,\"name\":\"Api\"}}},{\"name\":\"id\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":0,\"name\":\"ID\"}}},{\"name\":\"name\",\"type\":{\"kind\":0,\"name\":\"String\"}},{\"name\":\"rights\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":6,\"name\":\"Right\"}}}],\"name\":\"RolePermission\"}}}"sv;
		ASSERT_EQ( result, expected );
	}

	TEST_F( QLTests, IntrospectionDefTests )
	{
		//var defTest = format("{{ __type(name: \"{}\") {{ fields {{ name type {{ name kind ofType{{name kind}} }} }} }} }}"

		var defTest = format("{{ __type(name: \"{}\") {{ fields {{ name type {{ name kind ofType{{name kind}} }} }} }} }}", "RolePermission" );
		var defTestItems = DB::Query( defTest, 0 );
		LOG( ELogLevel::Debug, defTestItems.dump() );
		ASSERT_EQ( defTestItems.dump(), "{\"data\":{\"__type\":{\"fields\":[{\"name\":\"api\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":4,\"name\":\"Api\"}}},{\"name\":\"id\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":0,\"name\":\"ID\"}}},{\"name\":\"name\",\"type\":{\"kind\":0,\"name\":\"String\"}},{\"name\":\"rights\",\"type\":{\"kind\":7,\"name\":null,\"ofType\":{\"kind\":6,\"name\":\"Right\"}}}],\"name\":\"RolePermission\"}}}" );
	}

	TEST_F(QLTests, Filter)
	{
		var ql = "query{ rights(filter: {id: {ne: 0}}){ id name } }"sv;
		var items = DB::Query( ql, 0 );
		LOG( ELogLevel::Debug, items.dump() );
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

	TEST_F(QLTests, FetchUser)
	{
		constexpr sv ql = "query{ user(name:\"JohnSmith@google.com\") { id name attributes created deleted updated description target authenticator } }";
		var db = DB::Query( ql, 0 );
		//LOG( ELogLevel::Debug, db.dump() );
		ASSERT_EQ( db.dump(), "{\"data\":{\"user\":{\"authenticator\":\"Google\",\"created\":\"2021-02-13T09:01:25Z\",\"deleted\":\"2021-02-13T09:01:25Z\",\"description\":\"Unit Test User update\",\"id\":3,\"name\":\"JohnSmith@google.com\",\"target\":\"jsmith\",\"updated\":\"2021-02-13T09:01:25Z\"}}}" );
	}

	TEST_F(QLTests, DefTestsFetch)
	{
		var ql = "query{ role(target:\"user_management\"){ id name created deleted updated description target groups{id name created deleted updated description target } rolePermissions{api{ id name } id name rights } } }"sv;
		var db = DB::Query( ql, 0 );
		//LOG( ELogLevel::Debug, db.dump() );

		auto expected = "{\"data\":{\"role\":{\"created\":\"2021-02-13T07:00:33Z\",\"groups\":[{\"created\":\"2021-02-13T07:00:33Z\",\"id\":1,\"name\":\"Everyone\",\"target\":\"everyone\"},{\"created\":\"2021-02-13T07:00:33Z\",\"id\":2,\"name\":\"Users\",\"target\":\"users\"}],\"id\":1,\"name\":\"User Management\",\"rolePermissions\":[{\"api\":{\"id\":1,\"name\":\"UM\"},\"id\":1,\"rights\":[\"Administer\",\"Write\",\"Read\"]}],\"target\":\"user_management\"}}}"_json;
		SetAttribute( db, expected, "created" );
		SetAttribute( db, expected, "description" );
		SetAttribute( db, expected, "id" );
		var expected2 = expected.dump(); var actual = db.dump();
		ASSERT_EQ( actual, expected2 );
	}

	TEST_F(QLTests, ObjectFetch)
	{
		var ql = "query{ permissions(filter: null) {api{name} id name} }"sv;
		var actual = DB::Query( ql, 0 ).dump();
		LOG( ELogLevel::Debug, actual );
		auto expected = "{\"data\":{\"permissions\":[{\"api\":{\"name\":\"UM\"},\"id\":1},{\"api\":{\"name\":\"Tws\"},\"id\":2}]}}";
		ASSERT_EQ( actual, expected );
	}


	TEST_F(QLTests, EnumFetchApi)
	{
		var ql = "query{ __type(name: \"Api\") { enumValues { name description } } }"sv;
		var items = DB::Query( ql, 0 );
		LOG( ELogLevel::Debug, items.dump() );
		ASSERT_EQ( items.dump(), "{\"data\":{\"__type\":{\"enumValues\":[{\"name\":\"None\"},{\"name\":\"UM\"},{\"name\":\"Web\"},{\"name\":\"Tws\"},{\"name\":\"Blockly\"}]}}}" );
	}

	TEST_F(QLTests, EnumFetchAuthenticator)
	{
		var ql = "query{ __type(name: \"Authenticator\") { enumValues { id name } } }"sv;
		var items = DB::Query( ql, 0 );
		LOG( ELogLevel::Debug, items.dump() );
		ASSERT_EQ( items.dump(), "{\"data\":{\"__type\":{\"enumValues\":[{\"id\":0,\"name\":\"None\"},{\"id\":1,\"name\":\"Google\"}]}}}" );
	}

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
		//var x = items.dump();
		//LOG( ELogLevel::Debug, x );
		ASSERT_EQ( 0, 0 );
	}
}
