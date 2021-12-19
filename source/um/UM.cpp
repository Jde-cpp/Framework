#include "UM.h"
#include <boost/container/flat_set.hpp>
#include <jde/Str.h>
#include <jde/io/File.h>
#include "../Settings.h"
#include "../db/DataSource.h"
#include "../db/Database.h"
#include "../db/GraphQL.h"
#include "../db/Syntax.h"

#define var const auto
namespace Jde
{
	static const LogTag& _logLevel{ Logging::TagLevel("users") };
	using nlohmann::json;
	using boost::container::flat_set;
	namespace UM
	{
		using GroupPK=uint;
		using PermissionPK=uint;
		using RolePK=uint;
	}
	struct UMSettings
	{
		CIString Target{ "/UM/"sv };
		bool Use{ true };
		string LibraryName{"Jde.DB.Odbc.dll"};
		string ConnectionString;
		fs::path MetaDataPath;
	};
	α from_json( const json& j, UMSettings& settings )->void;
	sp<UMSettings> _pSettings;
	flat_map<string,UM::PermissionPK> _tablePermissions;
	flat_map<UserPK,flat_set<UM::GroupPK>> _userGroups; shared_mutex _userGroupMutex;
	flat_map<UM::GroupPK,flat_set<UM::RolePK>> _groupRoles; shared_mutex _groupRoleMutex;
	flat_map<UM::RolePK,flat_map<UM::PermissionPK,UM::EAccess>> _rolePermissions; shared_mutex _rolePermissionMutex;
	flat_map<UserPK, flat_map<UM::PermissionPK,UM::EAccess>> _userAccess; shared_mutex _userAccessMutex;

	flat_map<sv, UM::IAuthorize*> _authorizers; shared_mutex _authorizerMutex;
	UM::GroupAuthorize _groupAuthorize;
/*	struct IAuthorize
	{
		β TestAccess( EAccess access, UserPK userId, sv tableName )noexcept(false)->void=0;
	};*/
	α UM::AddAuthorizer( UM::IAuthorize* p )noexcept->void{ _authorizers.emplace( p->TableName, p ); }//pre-main
	α UM::FindAuthorizer( sv table )noexcept->IAuthorize*
	{
		auto p = _authorizers.find( table );
		return p==_authorizers.end() ? nullptr : p->second;
	}
	α AssignUser( UserPK userId, const flat_set<UM::GroupPK>& groupIds )->void
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
	α AssignUserGroups( UserPK userId=0 )noexcept(false)->void
	{
		ostringstream sql{ "select user_id, group_id from um_user_groups g join um_users u on g.user_id=u.id and u.deleted is null", std::ios::ate };
		vector<DB::object> params;
		if( userId )
		{
			sql << " where user_id=?";
			params.push_back( DB::object{userId} );
		}
		DB::DataSource().Select( sql.str(), [&](var& r)
		{
			unique_lock l{_userGroupMutex};
			_userGroups.try_emplace(r.GetUInt(0)).first->second.emplace( r.GetUInt(1) );
		}, params );
	}
	α UM::Configure()noexcept(false)->void
	{
		var& globalSettings = Settings::Global().Json();
		_pSettings = make_shared<UMSettings>();
		if( var pItem = globalSettings.find("um"); pItem!=globalSettings.end() )
			from_json( *pItem, *_pSettings );
		if( !_pSettings->Use )
			return;
		if( _pSettings->ConnectionString.empty() )
			_pSettings->ConnectionString = Settings::TryGet<string>( "db/connectionString" ).value_or( "" );
		THROW_IF( _pSettings->ConnectionString.empty(), "no user management connection string." );
		auto pDataSource = DB::DataSourcePtr(); CHECK( pDataSource );
		auto path = Settings::TryGet<fs::path>( "db/meta" ).value_or( "meta.json" );
		if( !fs::exists(path) )
			path = IApplication::ApplicationDataFolder()/path;
		LOG( "db meta='{}'"sv, path.string() );
		json j;
		try
		{
			j = json::parse( IO::FileUtilities::Load(path) );
		}
		catch( const std::exception& e )//nlohmann::detail::parse_error
		{
			throw IOException{ path, e.what(), SRCE_CUR };
		}
		auto& db = *pDataSource;
		var schema = db.SchemaProc()->CreateSchema( j, path.parent_path() );
		AppendQLSchema( schema );
		SetQLDataSource( pDataSource );

		var pApis = db.SelectEnumSync<uint>( "um_apis" );
		auto pId = Find( *pApis, "UM" ); THROW_IF( !pId, "no user management in api table." );
		var umPermissionId = db.Scaler<uint>( "select id from um_permissions where api_id=? and name is null", {*pId} ).value_or(0); THROW_IF( umPermissionId==0, "no user management permission." );
		for( var& table : schema.Tables )
			_tablePermissions.try_emplace( table.first, umPermissionId );

		AssignUserGroups();
		db.Select( "select permission_id, role_id, right_id from um_role_permissions p join um_roles r on p.role_id=r.id where r.deleted is null", [&](const DB::IRow& r){_rolePermissions.try_emplace(r.GetUInt(1)).first->second.emplace( r.GetUInt(0), (UM::EAccess)r.GetUInt(2) );} );
		db.Select( "select group_id, role_id from um_group_roles gr join um_groups g on gr.group_id=g.id and g.deleted is null join um_roles r on gr.role_id=r.id and r.deleted is null", [&](var& r){_groupRoles.try_emplace(r.GetUInt(0)).first->second.emplace( r.GetUInt(1) );} );
		for( var& [userId,groupIds] : _userGroups )
			AssignUser( userId, groupIds );
	}
	α IsTarget( sv url )noexcept{ return CIString{url}.starts_with(UMSettings().Target); }
	α UM::TestAccess( EAccess access, UserPK userId, PermissionPK permissionId )noexcept(false)->void
	{
		shared_lock l{ _userAccessMutex };
		var pUser = _userAccess.find( userId ); THROW_IF( pUser==_userAccess.end(), "User '{}' not found.", userId );
		var pAccess = pUser->second.find( permissionId ); THROW_IF( pAccess==pUser->second.end(), "User '{}' does not have api '{}' access.", userId, permissionId );
		THROW_IF( (pAccess->second & access)==EAccess::None, "User '{}' api '{}' access is limited to:  '{}'. requested:  '{}'.", userId, permissionId, (uint8)pAccess->second, (uint8)access );
	}
	α TestAccess( str tableName, UserPK userId, UM::EAccess access )noexcept(false)->void
	{
		var pTable = _tablePermissions.find( tableName ); THROW_IF( pTable==_tablePermissions.end(), "Could not find table '{}'", tableName );
		TestAccess( access, userId, pTable->second );
	}
	α UM::TestRead( str tableName, UserPK userId )noexcept(false)->void
	{
		Jde::TestAccess( tableName, userId, EAccess::Read );
	}

	α UM::ApplyMutation( const DB::MutationQL& m, PK id )noexcept(false)->void
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
				var pPermissionId = m.Args.find("permissionId"); THROW_IF( pPermissionId==m.Args.end(), "could not find permissionId in mutation" );
				var pRoleId = m.Args.find("roleId"); THROW_IF( pRoleId==m.Args.end(), "could not find roleId in mutation" );
				roleId = pRoleId->get<uint>();
				permissionId = pPermissionId->get<uint>();
			}
			else
			{
				roleId = m.InputParam( "roleId" ).get<uint>();
				permissionId = m.InputParam( "permissionId" ).get<uint>();
			}
			var access = (UM::EAccess)DB::DataSource().Scaler<uint>( "select right_id from um_role_permissions where role_id=? and permission_id=?", std::vector<DB::object>{roleId, permissionId} ).value_or( 0 );
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

	α from_json( const json& j, UMSettings& settings )->void
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
namespace Jde::UM
{
#pragma warning(disable:4100)
	α IAuthorize::TestPurge( uint pk, UserPK userId, SL sl )noexcept(false)->void
	{
		THROW_IFSL( !CanPurge(pk, userId), "Access to purge record denied" );
	}
}