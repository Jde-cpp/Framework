﻿#include "UM.h"
#include <boost/container/flat_set.hpp>
#include <jde/Str.h>
#include <jde/io/File.h>
#include <jde/io/Json.h>
#include "../Settings.h"
#include "../db/DataSource.h"
#include "../db/Database.h"
#include "../db/GraphQL.h"
#include "../db/Syntax.h"
#include "../coroutine/Awaitable.h"

#define var const auto
namespace Jde{
	static sp<LogTag> _logTag{ Logging::Tag("users") };
	using nlohmann::json;
	using boost::container::flat_set;
	namespace UM{
		using GroupPK=uint;
		using PermissionPK=uint;
		using RolePK=uint;
	}
	struct UMSettings{
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

	α UM::AddAuthorizer( UM::IAuthorize* p )ι->void{ _authorizers.emplace( p->TableName, p ); }//pre-main
	α UM::FindAuthorizer( sv table )ι->IAuthorize*{
		auto p = _authorizers.find( table );
		return p==_authorizers.end() ? nullptr : p->second;
	}
	α AssignUser( UserPK userId, const flat_set<UM::GroupPK>& groupIds )->void{
		for( var groupId : groupIds ){
			var pGroupRoles = _groupRoles.find(groupId); if( pGroupRoles==_groupRoles.end() ) continue;
			for( var roleId : pGroupRoles->second ){
				sl _{_rolePermissionMutex};
				var pRolePermissions = _rolePermissions.find( roleId ); if( pRolePermissions==_rolePermissions.end() ) continue;
				for( var& [permissionPK, access] : pRolePermissions->second )
					_userAccess.try_emplace( userId ).first->second.try_emplace( permissionPK, access );
			}
		}
	}
	α AssignUserGroups( UserPK userId=0 )ε->void{
		ostringstream sql{ "select member_id, entity_id from um_groups g join um_entities e on g.member_id=e.id and e.deleted is null", std::ios::ate };
		vector<DB::object> params;
		if( userId ){
			sql << " where user_id=?";//TODO - no user_id column
			params.push_back( DB::object{userId} );
		}
		DB::DataSource().Select( sql.str(), [&](var& r){
			unique_lock l{_userGroupMutex};
			_userGroups.try_emplace(r.GetUInt32(0)).first->second.emplace( r.GetUInt(1) );
		}, params );
	}
	α SetRolePermissions()ε->void{
		unique_lock _{ _rolePermissionMutex };
		_rolePermissions.clear();
		DB::DataSource().Select( "select permission_id, role_id, right_id from um_role_permissions p join um_roles r on p.role_id=r.id where r.deleted is null", [&](const DB::IRow& r){_rolePermissions.try_emplace(r.GetUInt(1)).first->second.emplace( r.GetUInt(0), (UM::EAccess)r.GetUInt(2) );} );
	}
	α UM::Configure()ε->void{
		var& globalSettings = Settings::Global().Json();
		_pSettings = make_shared<UMSettings>();
		if( var pItem = globalSettings.find("um"); pItem!=globalSettings.end() )
			from_json( *pItem, *_pSettings );
		if( !_pSettings->Use )
			return;
		if( _pSettings->ConnectionString.empty() )
			_pSettings->ConnectionString = Settings::Get<string>( "db/connectionString" ).value_or( "" );
		THROW_IF( _pSettings->ConnectionString.empty(), "no user management connection string." );
		auto pDataSource = DB::DataSourcePtr(); CHECK( pDataSource );
		fs::path path{ Settings::Env("db/meta").value_or("meta.json") };
		if( !fs::exists(path) )
			path = _msvc && _debug ? "../config/meta.json" : IApplication::ApplicationDataFolder()/path.stem();
		json j;
		try{
			j = Json::Parse( IO::FileUtilities::Load(path) );
		}
		catch( const IOException& e ){
			THROW( "Could not load db meta - {}", e.what() );
		}
		catch( const std::exception& e ){//nlohmann::detail::parse_error
			throw IOException{ path, e.what() };
		}
		auto& db = *pDataSource;
		auto sqlPath = Settings::Get<fs::path>( "db/scriptDir" ).value_or( "" );
		if( sqlPath.empty() )
			sqlPath = _msvc && _debug ? path.parent_path().parent_path() : path.parent_path();
		var schema = db.SchemaProc()->CreateSchema( j, sqlPath );
		AppendQLDBSchema( schema );
		SetQLDataSource( pDataSource );

		fs::path qlSchema{ Settings::Env("db/qlSchema").value_or("ql.json") };
		if( !fs::exists(qlSchema) )
			qlSchema = _msvc && _debug ? "../config/meta.json" : IApplication::ApplicationDataFolder()/path.stem();
		try{
			DB::SetQLIntrospection( Json::Parse(IO::FileUtilities::Load(qlSchema)) );
		}
		catch( const IOException& e ){
			THROW( "Could not load ql meta - {}", e.what() );
		}
		catch( const std::exception& e ){//nlohmann::detail::parse_error
			throw IOException{ path, e.what() };
		}

		var pApis = db.SelectEnumSync<uint,string>( "um_apis" );
		auto pId = FindKey( *pApis, "UM" ); THROW_IF( !pId, "no user management in api table." );
		var umPermissionId = db.Scaler<uint>( "select id from um_permissions where api_id=? and name is null", {*pId} ).value_or(0); THROW_IF( umPermissionId==0, "no user management permission." );
		for( var& table : schema.Tables )
			_tablePermissions.try_emplace( table.first, umPermissionId );

		AssignUserGroups();
		SetRolePermissions();
		db.Select( "select entity_id, role_id from um_entity_roles er join um_entities e on er.entity_id=e.id and e.deleted is null join um_roles r on er.role_id=r.id and r.deleted is null", [&](var& r){_groupRoles.try_emplace(r.GetUInt(0)).first->second.emplace( r.GetUInt(1) );} );
		for( var& [userId,groupIds] : _userGroups )
			AssignUser( userId, groupIds );
	}
	α IsTarget( sv url )ι{ return CIString{url}.starts_with(UMSettings().Target); }
	α UM::TestAccess( EAccess access, UserPK userId, PermissionPK permissionId )ε->void{
		sl _{ _userAccessMutex };
		var pUser = _userAccess.find( userId ); THROW_IF( pUser==_userAccess.end(), "User '{}' not found.", userId );
		var pAccess = pUser->second.find( permissionId ); THROW_IF( pAccess==pUser->second.end(), "User '{}' does not have api '{}' access.", userId, permissionId );
		THROW_IF( (pAccess->second & access)==EAccess::None, "User '{}' api '{}' access is limited to:  '{}'. requested:  '{}'.", userId, permissionId, (uint8)pAccess->second, (uint8)access );
	}
	α TestAccess( str tableName, UserPK userId, UM::EAccess access )ε->void{
		var pTable = _tablePermissions.find( tableName ); THROW_IF( pTable==_tablePermissions.end(), "Could not find table '{}'", tableName );
		TestAccess( access, userId, pTable->second );
	}
	α UM::TestRead( str tableName, UserPK userId )ε->void{
		Jde::TestAccess( tableName, userId, EAccess::Read );
	}

	α UM::ApplyMutation( const DB::MutationQL& m, UserPK id )ε->void{
		if( m.JsonName=="user" ){
			if( m.Type==DB::EMutationQL::Create /*|| m.Type==DB::EMutationQL::Restore*/ ){
				{ unique_lock l{ _userAccessMutex }; _userAccess.try_emplace( id ); }
				// if( m.Type==DB::EMutationQL::Restore ){
				// 	AssignUserGroups( id );
				// 	{ unique_lock l{ _userGroupMutex }; AssignUser( id, _userGroups[id] ); }
				// }
			}
			else if( /*m.Type==DB::EMutationQL::Delete ||*/ m.Type==DB::EMutationQL::Purge ){
				{ unique_lock l{ _userAccessMutex }; _userAccess.erase( (UserPK)id ); }
				{ unique_lock l{ _userGroupMutex }; _userGroups.erase( (UserPK)id ); }
			}
		}
		else if( m.JsonName=="groupRole" ){
			var groupId = m.InputParam( "groupId" ).get<uint>(); var roleId = m.InputParam( "roleId" ).get<uint>();
			unique_lock l{ _groupRoleMutex };
			if( m.Type==DB::EMutationQL::Add )
				_groupRoles.try_emplace(groupId).first->second.emplace( roleId );
			else if( var p = _groupRoles.find(groupId); m.Type==DB::EMutationQL::Remove && p != _groupRoles.end() )
				p->second.erase( std::remove_if(p->second.begin(), p->second.end(), [&](PK roleId2){return roleId2==roleId;}), p->second.end() );
		}
		else if( m.JsonName=="rolePermission" ){
			SetRolePermissions();
			var pRoleId = m.Args.find( "roleId" ); THROW_IF( pRoleId==m.Args.end(), "could not find roleId in mutation" );
			//uint roleId, permissionId;
			//if( m.Type==DB::EMutationQL::Update )
			//{
			//	var pPermissionId = m.Args.find("permissionId"); THROW_IF( pPermissionId==m.Args.end(), "could not find permissionId in mutation" );
			//	var pRoleId = m.Args.find("roleId"); THROW_IF( pRoleId==m.Args.end(), "could not find roleId in mutation" );
			//	roleId = pRoleId->get<uint>();
			//	permissionId = pPermissionId->get<uint>();
			//	var access = (UM::EAccess)DB::DataSource().Scaler<uint>( "select right_id from um_role_permissions where role_id=? and permission_id=?", std::vector<DB::object>{roleId, permissionId} ).value_or( 0 );
			//	{
			//		unique_lock l{ _rolePermissionMutex };
			//		_rolePermissions.try_emplace( roleId ).first->second[permissionId] = (Jde::UM::EAccess)access;
			//	}
			//}
			//else
			//	SetRolePermissions();
			flat_set<uint> groupIds;
			{
				sl _{ _groupRoleMutex };
				for( var& [groupId,roleIds] : _groupRoles )
				{
					if( roleIds.find(*pRoleId)!=roleIds.end() )
						groupIds.emplace( groupId );
				}
			}
			flat_set<uint> userIds;
			{
				sl _{_userGroupMutex};
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

	α from_json( const json& j, UMSettings& settings )->void{
		if( j.find("target")!=j.end() )
			j.at("target").get_to( settings.Target );
		if( j.find("use")!=j.end() )
			j.at("use").get_to( settings.Use );
		if( j.find("connectionString")!=j.end() ){
			string connectionString;
			j.at("connectionString").get_to( connectionString );
			settings.ConnectionString = OSApp::EnvironmentVariable( connectionString ).value_or( "" );
		}
		if( j.find("libraryName")!=j.end() )
			j.at("libraryName").get_to( settings.LibraryName );
		if( j.find("metaDataPath")!=j.end() ){
			string path;
			j.at("metaDataPath").get_to( path );
			settings.MetaDataPath = fs::path{ path };
		}
	}
	α LoginTask( string&& loginName, uint providerId, string&& opcServer, HCoroutine h )ι->Task{
		var opcServerParam = opcServer.size() ? DB::object{opcServer} : DB::object{nullptr};
		vector<DB::object> parameters = { move(loginName), providerId };
		if( opcServer.size() )
			parameters.push_back( opcServer );
		var sql = Ƒ( "select e.id from um_entities e join um_users u on e.id=u.entity_id join um_providers p on p.id=e.provider_id where u.login_name=? and p.id=? and p.target{}", opcServer.size() ? "=?" : " is null" );
		try{
			auto task = DB::ScalerCo<UserPK>( string{sql}, parameters );
			auto p = (co_await task).UP<UserPK>(); //gcc compile issue
			auto userId = p ? *p : 0;
			if( !userId ){
				if( !opcServer.size() )
					parameters.push_back( move(opcServer) );
				DB::ExecuteProc( "um_user_insert_login(?,?,?)", move(parameters), [&userId](var& row){userId=row.GetUInt32(0);} );
			}
			h.promise().SetResult( mu<UserPK>(userId) );
		}
		catch( IException& e ){
			h.promise().SetResult( move(e) );
		}
		h.resume();
	}
	α UM::Login( string loginName, uint providerId, string opcServer, SL sl )ι->AsyncAwait{
		//auto f = [l=move(loginName), type, o=move(opcServer)](HCoroutine h)mutable{ LoginTask(move(l), type, move(o), move(h)); };
		return AsyncAwait{ [l=move(loginName), providerId, o=move(opcServer)](HCoroutine h)mutable{ return LoginTask(move(l), providerId, move(o), move(h)); }, sl, "UM::Login" };
	}
}
namespace Jde::UM{
	LoginAwait::LoginAwait( vector<unsigned char> modulus, vector<unsigned char> exponent, string&& name, string&& target, string&& description, SL sl )ι:
			base{sl}, _modulus{ move(modulus) }, _exponent{ move(exponent) }, _name{ move(name) }, _target{ move(target) }, _description{ move(description) }
	{}

	α LoginAwait::Suspend()ι->void{
		LoginTask();
	}

	α LoginAwait::LoginTask()ε->Jde::Task{
		auto modulusHex = Str::ToHex( (byte*)_modulus.data(), _modulus.size() );
		try{
			THROW_IF( modulusHex.size() > 1024, "modulus {} is too long. max length: {}", modulusHex.size(), 1024 );
			uint32_t exponent{};
			for( var i : _exponent )
				exponent = (exponent<<8) | i;

			vector<DB::object> parameters = { modulusHex, exponent, underlying(EProviderType::Key) };
			var sql = "select e.id from um_entities e join um_users u on e.id=u.entity_id where u.modulus=? and u.exponent=? and e.provider_id=?";
			auto task = DB::ScalerCo<UserPK>( string{sql}, parameters );
			auto p = ( co_await task ).UP<UserPK>(); //gcc compile issue
			auto userPK = p ? *p : 0;
			if( !userPK ){
				parameters.push_back( move(_name) );
				parameters.push_back( move(_target) );
				parameters.push_back( move(_description) );
				DB::ExecuteProc( "um_user_insert_key(?,?,?,?,?,?)", move(parameters), [&userPK](var& row){userPK=row.GetUInt32(0);} );
			}
			Promise()->Resume( move(userPK), _h );
		}
		catch( IException& e ){
			Promise()->ResumeWithError( move(e), _h );
		}
	}

#pragma warning(disable:4100)
	α IAuthorize::TestPurge( uint pk, UserPK userId, SL sl )ε->void{
		THROW_IFSL( !CanPurge(pk, userId), "Access to purge record denied" );
	}
}