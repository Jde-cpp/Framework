#include "UM.h"
#include <boost/container/flat_set.hpp>
#include "../Settings.h"
#include <jde/Str.h>
#include "../db/DataSource.h"
#include "../db/Database.h"
#include "../db/GraphQL.h"
#include "../db/Syntax.h"
#include <jde/io/File.h>

#define var const auto
namespace Jde
{
	using boost::container::flat_set;
	namespace UM
	{
		typedef uint GroupPK;
		typedef uint PermissionPK;
		typedef uint RolePK;
	}
	struct UMSettings
	{
		CIString Target{ "/UM/"sv };
		bool Use{false};
		string LibraryName{"Jde.DB.Odbc.dll"};
		string ConnectionString;
		fs::path MetaDataPath;
	};
	using nlohmann::json;
	void from_json( const json& j, UMSettings& settings );
	sp<UMSettings> _pSettings;
	//flat_map<string,uint> _apis;
	flat_map<string,UM::PermissionPK> _tablePermissions;
	flat_map<UserPK,flat_set<UM::GroupPK>> _userGroups; shared_mutex _userGroupMutex;
	flat_map<UM::GroupPK,flat_set<UM::RolePK>> _groupRoles; shared_mutex _groupRoleMutex;
	//flat_map<UM::ApiPK,flat_map<string,UM::PermissionPK>> _permissions; shared_mutex _permissionMutex;

	flat_map<UM::RolePK,flat_map<UM::PermissionPK,UM::EAccess>> _rolePermissions; shared_mutex _rolePermissionMutex;
	//update _rolePermissions
	//for each role group, reassign user.


	flat_map<UserPK, flat_map<UM::PermissionPK,UM::EAccess>> _userAccess; shared_mutex _userAccessMutex;
	void AssignUser( UserPK userId, const flat_set<UM::GroupPK>& groupIds )
	{
		for( var groupId : groupIds )
		{
			var pGroupRoles = _groupRoles.find(groupId); if( pGroupRoles==_groupRoles.end() ) continue;
			for( var roleId : pGroupRoles->second )
			{
				shared_lock l{_rolePermissionMutex};
				var pRolePermissions = _rolePermissions.find( roleId ); if( pRolePermissions==_rolePermissions.end() ) continue;
				for( var& [permissionPK, access] : pRolePermissions->second )
					_userAccess.try_emplace( userId ).first->second.try_emplace( permissionPK, access );
			}
		}
	}
	void AssignUserGroups( UserPK userId=0 )noexcept(false)
	{
		ostringstream sql{ "select user_id, group_id from um_user_groups g join um_users u on g.user_id=u.id and u.deleted is null", std::ios::ate };
		vector<DB::DataValue> params;
		if( userId )
		{
			sql << " where user_id=?";
			params.push_back( DB::DataValue{userId} );
		}
		QUERY.Select( sql.str(), [&](var& r)
		{
			unique_lock l{_userGroupMutex};
			_userGroups.try_emplace(r.GetUInt(0)).first->second.emplace( r.GetUInt(1) );
		}, params );
	}
	void UM::Configure()noexcept(false)
	{
		var& globalSettings = Settings::Global().Json();
		_pSettings = make_shared<UMSettings>();
		if( var pItem = globalSettings.find("um"); pItem!=globalSettings.end() )
			from_json( *pItem, *_pSettings );
		if( !_pSettings->Use )
			return;
		if( _pSettings->ConnectionString.empty() )
			_pSettings->ConnectionString = Settings::Global().Get2<string>( "connectionString" ).value_or( "" );
		THROW_IF( _pSettings->ConnectionString.empty(), EnvironmentException("no user management connection string.") );
		auto pDataSource = DB::DataSource();// _pSettings->LibraryName, _pSettings->ConnectionString );
		var path = Settings::Global().Get<fs::path>( "metaDataPath" );
		INFO( "db meta='{}'"sv, path.string() );
		var j = json::parse( IO::FileUtilities::Load(path) );
		var schema = pDataSource->SchemaProc()->CreateSchema( j );
		AppendQLSchema( schema );
		SetQLDataSource( pDataSource );

		var pApis = pDataSource->SelectMap<string,uint>( "select name, id from um_apis" );
		auto pUMApi = pApis->find( "UM" ); THROW_IF( pUMApi==pApis->end(), EnvironmentException("no user management in api table.") );
		var umPermissionId = pDataSource->Scaler<uint>( "select id from um_permissions where api_id=? and name is null", {pUMApi->second} ).value_or(0); THROW_IF( umPermissionId==0, EnvironmentException("no user management permission.") );
		for( var& table : schema.Tables )
			_tablePermissions.try_emplace( table.first, umPermissionId );

		AssignUserGroups();
		pDataSource->Select( "select permission_id, role_id, right_id from um_role_permissions p join um_roles r on p.role_id=r.id where r.deleted is null", [&](const DB::IRow& r){_rolePermissions.try_emplace(r.GetUInt(1)).first->second.emplace( r.GetUInt(0), (UM::EAccess)r.GetUInt(2) );} );
		pDataSource->Select( "select group_id, role_id from um_group_roles gr join um_groups g on gr.group_id=g.id and g.deleted is null join um_roles r on gr.role_id=r.id and r.deleted is null", [&](var& r){_groupRoles.try_emplace(r.GetUInt(0)).first->second.emplace( r.GetUInt(1) );} );
		for( var& [userId,groupIds] : _userGroups )
			AssignUser( userId, groupIds );
	}
	bool IsTarget( sv url )noexcept{ return CIString{url}.starts_with(UMSettings().Target); }
	void UM::TestAccess( EAccess access, UserPK userId, PermissionPK permissionId )noexcept(false)
	{
		shared_lock l{ _userAccessMutex };
		var pUser = _userAccess.find( userId ); THROW_IF( pUser==_userAccess.end(), Exception("User '{}' not found.",userId) );
		var pAccess = pUser->second.find( permissionId ); THROW_IF( pAccess==pUser->second.end(), Exception("User '{}' does not have api '{}' access.", userId, permissionId) );
		THROW_IF( (pAccess->second & access)==EAccess::None, Exception("User '{}' api '{}' access is limited to:  '{}'. requested:  '{}'.", userId, permissionId, (uint8)pAccess->second, (uint8)access) );
	}
	void UM::TestAccess( EAccess access, UserPK userId, sv tableName )noexcept(false)
	{
		var pTable = _tablePermissions.find(string{tableName}); THROW_IF( pTable==_tablePermissions.end(), Exception("Could not find table '{}'", tableName) );
		TestAccess( access, userId, pTable->second );
	}

	void UM::ApplyMutation( const DB::MutationQL& m, PK id )noexcept(false)
	{
		if( m.JsonName=="user" )
		{
			if( m.Type==DB::EMutationQL::Create || m.Type==DB::EMutationQL::Restore )
			{
				{ unique_lock l{ _userAccessMutex }; _userAccess.try_emplace( id ); }
				if( m.Type==DB::EMutationQL::Restore )
				{
					AssignUserGroups( id );
					{ unique_lock l{ _userGroupMutex }; AssignUser( id, _userGroups[id] ); }
				}
			}
			else if( m.Type==DB::EMutationQL::Delete || m.Type==DB::EMutationQL::Purge )
			{
				{ unique_lock l{ _userAccessMutex }; _userAccess.erase( id ); }
				{ unique_lock l{ _userGroupMutex }; _userGroups.erase( id ); }
			}
		}
		else if( m.JsonName=="groupRole" )
		{
			var groupId = m.InputParam( "groupId" ).get<uint>(); var roleId = m.InputParam( "roleId" ).get<uint>();
			unique_lock l{ _groupRoleMutex };
			if( m.Type==DB::EMutationQL::Add )
				_groupRoles.try_emplace(groupId).first->second.emplace( roleId );
			else if( var p = _groupRoles.find(groupId); m.Type==DB::EMutationQL::Remove && p != _groupRoles.end() )
				p->second.erase( std::remove_if(p->second.begin(), p->second.end(), [&](PK roleId2){return roleId2==roleId;}), p->second.end() );
		}
		else if( m.JsonName=="rolePermission" )
		{
			uint roleId, permissionId;
			if( m.Type==DB::EMutationQL::Update )
			{
				var pPermissionId = m.Args.find("permissionId"); THROW_IF( pPermissionId==m.Args.end(), Exception("could not find permissionId in mutation") );
				var pRoleId = m.Args.find("roleId"); THROW_IF( pRoleId==m.Args.end(), Exception("could not find roleId in mutation") );
				roleId = pRoleId->get<uint>();
				permissionId = pPermissionId->get<uint>();
			}
			else
			{
				roleId = m.InputParam( "roleId" ).get<uint>();
				permissionId = m.InputParam( "permissionId" ).get<uint>();
			}
			var access = (UM::EAccess)DB::DataSource()->Scaler<uint>( "select right_id from um_role_permissions where role_id=? and permission_id=?", std::vector<DB::DataValue>{roleId, permissionId} ).value_or( 0 );
			{
				unique_lock l{ _rolePermissionMutex };
				_rolePermissions.try_emplace( roleId ).first->second[permissionId] = (Jde::UM::EAccess)access;
			}
			flat_set<uint> groupIds;
			{
				shared_lock l{ _groupRoleMutex };
				for( var& [groupId,roleIds] : _groupRoles )
				{
					if( find(roleIds.begin(), roleIds.end(), roleId)!=roleIds.end() )
						groupIds.emplace( groupId );
				}
			}
			flat_set<uint> userIds;
			{
				shared_lock l{_userGroupMutex};
				for( var& [userId,userGroupIds] : _userGroups )
				{
					for( var permissionGroupId : groupIds )
					{
						if( userGroupIds.find(permissionGroupId)!=userGroupIds.end() )
						{
							AssignUser( userId, userGroupIds );
							break;
						}
					}
				}
			}
		}
	}
/*
#define TEST_ACCESS(a,b,c) DBG( "TEST_ACCESS({},{},{})"sv, a, b, c )
	string Get( sv url, UserPK userId )noexcept(false)
	{
		TEST_ACCESS( "Get", url, userId ); //TODO implement.
		//throw "not implemented";
		return {};
	}
*/
	void from_json( const json& j, UMSettings& settings )
	{
		if( j.find("target")!=j.end() )
			j.at("target").get_to( settings.Target );
		if( j.find("use")!=j.end() )
			j.at("use").get_to( settings.Use );
		if( j.find("connectionString")!=j.end() )
		{
			string connectionString;
			j.at("connectionString").get_to( connectionString );
			settings.ConnectionString = IApplication::Instance().GetEnvironmentVariable( connectionString );
		}
		if( j.find("libraryName")!=j.end() )
			j.at("libraryName").get_to( settings.LibraryName );
		if( j.find("metaDataPath")!=j.end() )
		{
			string path;
			j.at("metaDataPath").get_to( path );
			settings.MetaDataPath = fs::path{ path };
		}
	}
}