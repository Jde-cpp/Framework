#include "DiskWatcher.h"
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

namespace Jde::IO
{
#ifndef _MSC_VER
	NotifyEvent::NotifyEvent( const inotify_event& sys ):
		WatchDescriptor{ sys.wd },
		Mask{ (EDiskWatcherEvents)sys.mask },
		Cookie{ sys.cookie },
		Name{ string(sys.len ? (char*)&sys.name : "") }
	{}
#endif
	std::ostream& operator<<( std::ostream& os, const NotifyEvent& event )
	{
		std::ios_base::fmtflags f( os.flags() );
		os << "wd=" << event.WatchDescriptor
			<< ";  mask=" << std::hex << (uint)event.Mask;
		os.flags( f );
		os	<< ";  cookie=" << event.Cookie
			<< ";  name='" << event.Name << "'";

		return os;
	}
#ifndef _MSC_VER
	DiskWatcher::DiskWatcher( path path, EDiskWatcherEvents events )noexcept(false):
		_events{events},
		_path(path),
		_fd{ ::inotify_init1(IN_NONBLOCK) }
	{
		THROW_IFX2( _fd < 0, IO_EX(path, "({})Could not init inotify."sv, errno) );
		TRACE( "DiskWatcher::inotify_init returned {}"sv, _fd );

		try
		{
			auto add = [&]( var& subPath )
			{
				var wd = ::inotify_add_watch( _fd, subPath.string().c_str(), (uint32_t)_events );
				THROW_IFX2( wd < 0, IO_EX(subPath, "({})Could not init watch.", errno) );
				_descriptors.emplace( wd, subPath );
				TRACE( "inotify_add_watch on {}, returned {}"sv, subPath.c_str(), wd );
			};
			add( _path );
			// add recursive
			// auto pDirs = FileUtilities::GetDirectories( _path );
			// for( var& dir : *pDirs )
			// 	add( dir.path() );
		}
		catch( const IOException& e )
		{
			close(_fd);
			throw e;
		}
		DBG( "DiskWatcher::DiskWatcher on {}"sv, path.c_str() );
		_pThread = make_shared<Jde::Threading::InterruptibleThread>( "Diskwatcher", [&](){ Run();} );
		IApplication::AddThread( _pThread );
	}
	DiskWatcher::~DiskWatcher()
	{
		DBG( "DiskWatcher::~DiskWatcher on  {}"sv, _path );
		close( _fd );
	//	_pThread->Join();
	}

	void DiskWatcher::Run()noexcept
	{
		DBG( "DiskWatcher::Run  {}"sv, _path );
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
		IApplication::Remove( pThis );
	}

	void DiskWatcher::ReadEvent( const pollfd& fd, bool isRetry )noexcept(false)
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

			for( int i=0; i < length; )
			{
				struct inotify_event *pEvent = (struct inotify_event *) &buffer[i];
				string name( pEvent->len ? (char*)&pEvent->name : "" );
				//DBG0( name );
				DBG( "DiskWatcher::read=>wd={}, mask={}, len={}, cookie={}, name={}"sv, pEvent->wd, pEvent->mask, pEvent->len, pEvent->cookie, name );
				auto pDescriptorChange = _descriptors.find( pEvent->wd );
				if( pDescriptorChange==_descriptors.end() )
				{
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
	DiskWatcher::DiskWatcher( path /*path*/, EDiskWatcherEvents /*events=DefaultEvents*/ )noexcept(false)
	{
		THROW( "new NotImplemented" );
	}
	DiskWatcher::~DiskWatcher()
	{}
#endif


/*	DiskWatcher::DiskWatcher()noexcept(false):
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
	void DiskWatcher::ReadEvent( bool isRetry )noexcept(false)
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
			DBG( "DiskWatcher::read=>wd={}, mask={}, len={}, cookie={}, name={}", pEvent->wd, pEvent->mask, pEvent->len, pEvent->cookie, pEvent->len ? (char*)&pEvent->name : "" );
			var event = NotifyEvent( *pEvent );
			pNotifyEvents->push_front( event );
			i += EventSize + pEvent->len;
		}
		auto sendEvents = [&]( sp<std::forward_list<NotifyEvent>> pEvents )
		{
			for( var& event : *pEvents )
			{
				tuple<shared_ptr<IDriveChange>,fs::path>* pOnChangePath;
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
	void DiskWatcher::OnTimeout()noexcept
	{
		OnAwake();
	}
	void DiskWatcher::OnAwake()noexcept
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
	void IDriveChange::OnAccess( path path, const NotifyEvent& event )noexcept
	{
		LOG( _logLevel, "IDriveChange::OnAccess( {}, {} )", path.string(), event );
		//Logging::Log( Logging::MessageBase(_logLevel, "IDriveChange::OnAccess( {}, {} )", "__FILE__", "__func__", 201), path.string(), event );
	}
	void IDriveChange::OnModify( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnModify( {}, {} )", path.string(), event ); }
	void IDriveChange::OnAttribute( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnAttribute( {}, {} )", path.string(), event ); }
	void IDriveChange::OnCloseWrite( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnCloseWrite( {}, {} )", path.string(), event ); }
	void IDriveChange::OnCloseNoWrite( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnCloseNoWrite( {}, {} )", path.string(), event ); }
	void IDriveChange::OnOpen( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnOpen( {}, {} )", path.string(), event ); }
	void IDriveChange::OnMovedFrom( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnMovedFrom( {}, {} )", path.string(), event ); }
	void IDriveChange::OnMovedTo( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnMovedTo( {}, {} )", path.string(), event ); }
	void IDriveChange::OnCreate( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnCreate( {}, {} )", path.string(), event ); }
	void IDriveChange::OnDelete( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnDelete( {}, {} )", path.string(), event ); }
	void IDriveChange::OnDeleteSelf( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnDeleteSelf( {}, {} )", path.string(), event ); }
	void IDriveChange::OnMoveSelf( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnMoveSelf( {}, {} )", path.string(), event ); }
	void IDriveChange::OnUnmount( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnUnmount( {}, {} )", path.string(), event ); }
	void IDriveChange::OnQOverflow( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnQOverflow( {}, {} )", path.string(), event ); }
	void IDriveChange::OnIgnored( path path, const NotifyEvent& event )noexcept{ LOG( _logLevel, "IDriveChange::OnIgnored( {}, {} )", path.string(), event ); }
}
