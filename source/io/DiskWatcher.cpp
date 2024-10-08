﻿#include "DiskWatcher.h"
#ifndef _MSC_VER
	#include <sys/inotify.h>
	#include <poll.h>
#endif
#include <errno.h>
#include <forward_list>
#include <memory>

#include <jde/Exception.h>
#include <jde/io/File.h>
#include <jde/App.h>
#define var const auto

namespace Jde::IO{

	static sp<Jde::LogTag> _logTag = Logging::Tag( "diskWatcher" );
	α DiskWatcher::LogTag()ι->sp<Jde::LogTag>{ return _logTag; }
#ifndef _MSC_VER
	NotifyEvent::NotifyEvent( const inotify_event& sys )ι:
		WatchDescriptor{ sys.wd },
		Mask{ (EDiskWatcherEvents)sys.mask },
		Cookie{ sys.cookie },
		Name{ string(sys.len ? (char*)&sys.name : "") }
	{}
#endif
	α NotifyEvent::ToString()Ι->string{
		std::ostringstream os;
		std::ios_base::fmtflags f( os.flags() );
		os << "wd=" << WatchDescriptor
			<< ";  mask=" << std::hex << (uint)Mask;
		os.flags( f );
		os	<< ";  cookie=" << Cookie
			<< ";  name='" << Name << "'";

		return os.str();
	}
#ifndef _MSC_VER
	DiskWatcher::DiskWatcher( const fs::path& path, EDiskWatcherEvents events )ε:
		_events{events},
		_path(path),
		_fd{ ::inotify_init1(IN_NONBLOCK) }
	{
		THROW_IFX( _fd < 0, IO_EX(path, ELogLevel::Error, "({})Could not init inotify."sv, errno) );
		TRACE( "DiskWatcher::inotify_init returned {}", _fd );

		try
		{
			auto add = [&]( var& subPath )
			{
				var wd = ::inotify_add_watch( _fd, subPath.string().c_str(), (uint32_t)_events );
				THROW_IFX( wd < 0, IO_EX(subPath, ELogLevel::Error, "({})Could not init watch.", errno) );
				_descriptors.emplace( wd, subPath );
				TRACE( "inotify_add_watch on {}, returned {}"sv, subPath.c_str(), wd );
			};
			add( _path );
			// add recursive
			// auto pDirs = FileUtilities::GetDirectories( _path );
			// for( var& dir : *pDirs )
			// 	add( dir.path() );
		}
		catch( IOException& e )
		{
			close(_fd);
			throw move(e);
		}
		TRACE( "DiskWatcher::DiskWatcher on {}"sv, path.c_str() );
		_pThread = make_shared<Jde::Threading::InterruptibleThread>( "Diskwatcher", [&](){ Run();} );
		IApplication::AddThread( _pThread );
	}
	DiskWatcher::~DiskWatcher(){
		TRACE( "DiskWatcher::~DiskWatcher on  {}", _path.string() );
		close( _fd );
	}

	α DiskWatcher::Run()ι->void{
		TRACE( "DiskWatcher::Run  {}"sv, _path.string() );
		struct pollfd fds;
		fds.fd = _fd;
		fds.events = POLLIN;
		while( !Threading::GetThreadInterruptFlag().IsSet() )
		{
			var result = ::poll( &fds, 1, 5000 );
			if( result<0 )
			{
				var err = errno;
				if( err!=EINTR )
				{
					ERR( "poll returned {}."sv, err );
					break;
				}
				else
					ERR( "poll returned {}."sv, err );
			}
			else if( result>0 && (fds.revents & POLLIN) )
			{
				try
				{
					ReadEvent( fds );
				}
				catch( const Jde::IOException& )
				{}
			}
		}
		IApplication::RemoveThread( _pThread );
		auto pThis = shared_from_this();//keep alive
		Process::RemoveKeepAlive( pThis );
	}

	α DiskWatcher::ReadEvent( const pollfd& fd, bool isRetry )ε->void
	{
		for( ;; )
		{
			constexpr uint EventSize = sizeof( struct inotify_event );
			constexpr uint BufferLength = (EventSize+16)*1024;
			char buffer[BufferLength];

			var length = read( _fd, buffer, BufferLength );
			var err=errno;
			if( length==-1 && err!=EAGAIN )
			{
				if( err == EINTR && !isRetry )
				{
					WARN( "read return EINTR, retrying"sv );
					ReadEvent( fd, true );
				}
				THROW( "read return '{}'", err );
			}
			if( length<=0 )
				break;

			for( int i=0; i < length; ){
				struct inotify_event *pEvent = (struct inotify_event *) &buffer[i];
				string name( pEvent->len ? (char*)&pEvent->name : "" );
				//DBG0( name );
				TRACE( "DiskWatcher::read=>wd={}, mask={}, len={}, cookie={}, name={}"sv, pEvent->wd, pEvent->mask, pEvent->len, pEvent->cookie, name );
				auto pDescriptorChange = _descriptors.find( pEvent->wd );
				if( pDescriptorChange==_descriptors.end() ){
					ERR( "Could not find OnChange interface for wd '{}'"sv, pEvent->wd );
					continue;
				}
				var path = pDescriptorChange->second/name;
				var event = NotifyEvent( *pEvent );
				if( Any(event.Mask & EDiskWatcherEvents::Modify) )
					OnModify( path, event );
				if( Any(event.Mask & EDiskWatcherEvents::MovedFrom) )
					OnMovedFrom( path, event );
				if( Any(event.Mask & EDiskWatcherEvents::MovedTo) )
					OnMovedTo( path, event );
				if( Any(event.Mask & EDiskWatcherEvents::Create) )
					OnCreate( path, event );
				if( Any(event.Mask & EDiskWatcherEvents::Delete) )
					OnDelete( path, event );
				i += EventSize + pEvent->len;
			}
		}
	}
#else
	DiskWatcher::DiskWatcher( const fs::path& /*path*/, EDiskWatcherEvents /*events=DefaultEvents*/ )ε
	{
		THROW( "new NotImplemented" );
	}
	DiskWatcher::~DiskWatcher()
	{}
#endif


/*	DiskWatcher::DiskWatcher()ε:
		Interrupt( "DiskWatcher", 1ns )
	{
		if( !_fd )
		{
			_fd = inotify_init();
			if( _fd < 0 )
			{
				_fd = 0;
				THROW( IOException("({})Could not init inotify.", errno) );
			}
			else
				GetDefaultLogger()->info( "inotify_init returned {}", _fd );
		}
	}

	DiskWatcher::~DiskWatcher()
	{
		for( var& descriptorOnChange : _descriptors )
		{
			var wd = descriptorOnChange.first;
			var ret = inotify_rm_watch( _fd, wd );
			if( ret )
				ERR( "inotify_rm_watch( {}, {} ) returned '{}', continuing", _fd, wd, ret );
		}
		var ret = close( _fd );
		if( ret )
			ERR( "close( {} ) returned '{}', continuing", _fd, ret );
	}

	//https://www.linuxjournal.com/article/8478
	α DiskWatcher::ReadEvent( bool isRetry )ε
	{
		constexpr uint EventSize = sizeof( struct inotify_event );
		constexpr uint BufferLength = (EventSize+16)*1024;
		char buffer[BufferLength];

		var length = read( _fd, buffer, BufferLength );
		if( !length )
			THROW( IOException("BufferLength too small?") );
		if( length < 0 )
		{
			if( errno == EINTR && !isRetry )
			{
				WARN0( "read return EINTR, retrying" );
				ReadEvent( true );
			}
			else
				THROW( IOException("read return {}", errno) );
		}
		auto pNotifyEvents = ms<std::forward_list<NotifyEvent>>();
		for( int i=0; i < length; )
		{
			struct inotify_event *pEvent = (struct inotify_event *) &buffer[i];
			string name( pEvent->len ? (char*)&pEvent->name : "" );
			DBG0( name );
			TRACE( "DiskWatcher::read=>wd={}, mask={}, len={}, cookie={}, name={}", pEvent->wd, pEvent->mask, pEvent->len, pEvent->cookie, pEvent->len ? (char*)&pEvent->name : "" );
			var event = NotifyEvent( *pEvent );
			pNotifyEvents->push_front( event );
			i += EventSize + pEvent->len;
		}
		auto sendEvents = [&]( sp<std::forward_list<NotifyEvent>> pEvents )
		{
			for( var& event : *pEvents )
			{
				tuple<sp<IDriveChange>,fs::path>* pOnChangePath;
				{
					std::shared_lock<std::shared_mutex> l( _mutex );
					auto pDescriptorChange = _descriptors.find( event.WatchDescriptor );
					if( pDescriptorChange==_descriptors.end() )
					{
						ERR( "Could not find OnChange interface for wd '{}'", event.WatchDescriptor );
						continue;
					}
					pOnChangePath = &pDescriptorChange->second;
				}
				auto pOnChange = std::get<0>(*pOnChangePath);
				var path = std::get<1>( *pOnChangePath );
				if( Any(event.Mask & DiskWatcherEvents::Access) )
					pOnChange->OnAccess( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Modify) )
					pOnChange->OnModify( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Attribute) )
					pOnChange->OnAttribute( path, event );
				if( Any(event.Mask & DiskWatcherEvents::CloseWrite) )
					pOnChange->OnCloseWrite( path, event );
				if( Any(event.Mask & DiskWatcherEvents::CloseNoWrite) )
					pOnChange->OnCloseNoWrite( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Open) )
					pOnChange->OnOpen( path, event );
				if( Any(event.Mask & DiskWatcherEvents::MovedFrom) )
					pOnChange->OnMovedFrom( path, event );
				if( Any(event.Mask & DiskWatcherEvents::MovedTo) )
					pOnChange->OnMovedTo( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Create) )
					pOnChange->OnCreate( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Delete) )
					pOnChange->OnDelete( path, event );
				if( Any(event.Mask & DiskWatcherEvents::DeleteSelf) )
					pOnChange->OnDeleteSelf( path, event );
				if( Any(event.Mask & DiskWatcherEvents::MoveSelf) )
					pOnChange->OnMoveSelf( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Unmount) )
					pOnChange->OnUnmount( path, event );
				if( Any(event.Mask & DiskWatcherEvents::QOverflow) )
					pOnChange->OnQOverflow( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Ignored) )
					pOnChange->OnIgnored( path, event );
			}
		};
		std::thread thd( [pNotifyEvents,&sendEvents](){sendEvents( pNotifyEvents );} );
		thd.detach();
	}
	α DiskWatcher::OnTimeout()ι->void
	{
		OnAwake();
	}
	α DiskWatcher::OnAwake()ι
	{
		fd_set rfds; FD_ZERO( &rfds );
		FD_SET( _fd, &rfds );

		struct timeval time;
		time.tv_sec = _interuptCheckSeconds;
		time.tv_usec = 0;

		var ret = select( _fd + 1, &rfds, nullptr, nullptr, &time );
		if( ret < 0 )
			ERR( "select returned '{}' , errno='{}'", ret, errno );
		else if( ret && FD_ISSET(_fd, &rfds) )// not a timed out!
		{
			try
			{
				ReadEvent();
			}
			catch( const IOException& e )
			{
				ERR( "ReadEvent threw {}, continuing...", e );
			}
		}
	}
*/
	α IDriveChange::OnAccess( const fs::path& path, const NotifyEvent& event )ι->void{
		TRACE( "IDriveChange::OnAccess( {}, {} )", path.string(), event.ToString() );
		//Logging::Log( Logging::MessageBase(_logTag, "IDriveChange::OnAccess( {}, {} )", "__FILE__", "__func__", 201), path.string(), event );
	}
	α IDriveChange::OnModify( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnModify( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnAttribute( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnAttribute( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnCloseWrite( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnCloseWrite( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnCloseNoWrite( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnCloseNoWrite( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnOpen( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnOpen( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnMovedFrom( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnMovedFrom( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnMovedTo( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnMovedTo( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnCreate( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnCreate( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnDelete( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnDelete( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnDeleteSelf( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnDeleteSelf( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnMoveSelf( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnMoveSelf( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnUnmount( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnUnmount( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnQOverflow( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnQOverflow( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnIgnored( const fs::path& path, const NotifyEvent& event )ι->void{ TRACE( "IDriveChange::OnIgnored( {}, {} )", path.string(), event.ToString() ); }
}
