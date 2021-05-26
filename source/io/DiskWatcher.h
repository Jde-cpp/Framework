#pragma once
#include <jde/Exports.h>
#include "../threading/InterruptibleThread.h"
#include <shared_mutex>
struct inotify_event;
struct pollfd;
namespace Jde::IO
{
#pragma region DiskWatcherEvents

	enum class EDiskWatcherEvents : uint32_t
	{
		None         = 0x00000000,
		Access       = 0x00000001,	/* File was accessed */
		Modify       = 0x00000002,	/* File was modified */
		Attribute    = 0x00000004,	/* Metadata changed */
		CloseWrite   = 0x00000008,	/* Writtable file was closed */
		CloseNoWrite =	0x00000010,	/* Unwrittable file closed */
		Open         = 0x00000020,	/* File was opened */
		MovedFrom    = 0x00000040,	/* File was moved from X */
		MovedTo		 = 0x00000080,	/* File was moved to Y */
		Create		 = 0x00000100,	/* Subfile was created */
		Delete		 = 0x00000200,	/* Subfile was deleted */
		DeleteSelf	 =	0x00000400,	/* Self was deleted */
		MoveSelf		 = 0x00000800,	/* Self was moved */
											/* the following are legal events.  they are sent as needed to any watch */
											Unmount		 = 0x00002000,	/* Backing fs was unmounted */
											QOverflow	 =	0x00004000,	/* Event queued overflowed */
											Ignored		 = 0x00008000,	/* File was ignored */
																				/* special flags */
																				OnlyDir      =	0x01000000,	/* only watch the path if it is a directory */
																				DontFollow   = 0x02000000,	/* don't follow a sym link */
																				MaskAdd      = 0x20000000,	/* add to the mask of an already existing watch */
																				IsDir        = 0x40000000,	/* event occurred against dir */
																				OneShot      = 0x80000000,	/* only send event once */
																				AllEvents=Access+Modify+Attribute+CloseWrite+CloseNoWrite+Open+MovedFrom+MovedTo+Create+Delete+DeleteSelf+MoveSelf
	};
	constexpr inline EDiskWatcherEvents operator|(EDiskWatcherEvents a, EDiskWatcherEvents b){ return (EDiskWatcherEvents)( (uint32_t)a | (uint32_t)b ); }
	inline EDiskWatcherEvents operator&(EDiskWatcherEvents a, EDiskWatcherEvents b){ return (EDiskWatcherEvents)( (uint32_t)a & (uint32_t)b ); }
	inline bool Any(EDiskWatcherEvents a){ return a!=EDiskWatcherEvents::None; }
#pragma endregion
#pragma region NotifyEvent
	struct NotifyEvent
	{
		NotifyEvent( const inotify_event& sys );
		const int WatchDescriptor;
		const EDiskWatcherEvents Mask;
		const uint32_t Cookie;
		const string Name;
	};
	std::ostream& operator<<( std::ostream& os, const NotifyEvent& event );
#pragma endregion
#pragma region	IDriveChange
//#ifndef _MSC_VER
	struct JDE_NATIVE_VISIBILITY IDriveChange
	{
		virtual void OnAccess( path path, const NotifyEvent& event )noexcept;
		virtual void OnModify( path path, const NotifyEvent& event )noexcept;
		virtual void OnAttribute( path path, const NotifyEvent& event )noexcept;
		virtual void OnCloseWrite( path path, const NotifyEvent& event )noexcept;
		virtual void OnCloseNoWrite( path path, const NotifyEvent& event )noexcept;
		virtual void OnOpen( path path, const NotifyEvent& event )noexcept;
		virtual void OnMovedFrom( path path, const NotifyEvent& event )noexcept;
		virtual void OnMovedTo( path path, const NotifyEvent& event )noexcept;
		virtual void OnCreate( path path, const NotifyEvent& event )noexcept;
		virtual void OnDelete( path path, const NotifyEvent& event )noexcept;
		virtual void OnDeleteSelf( path path, const NotifyEvent& event )noexcept;
		virtual void OnMoveSelf( path path, const NotifyEvent& event )noexcept;
		virtual void OnUnmount( path path, const NotifyEvent& event )noexcept;
		virtual void OnQOverflow( path path, const NotifyEvent& event )noexcept;
		virtual void OnIgnored( path path, const NotifyEvent& event )noexcept;
	private:
		const ELogLevel _logLevel{ELogLevel::Debug};
	};
//#endif
#pragma endregion
	struct WatcherSettings
	{
		Duration Delay;//{ 1min }
		vector<string> ExcludedRegEx;
	};
	enum EFileFlags
	{
		None = 0x0,
		Hidden = 0x2,
		System = 0x4,
		Directory = 0x10,
		Archive = 0x20,
		Temporary = 0x100
	};
	struct IDirEntry
	{
		IDirEntry()=default;
		IDirEntry( EFileFlags flags, path path, uint size, const TimePoint& createTime=TimePoint(), const TimePoint& modifyTime=TimePoint() ):
			Flags{ flags },
			Path{ path },
			Size{ size },
			CreatedTime{ createTime },
			ModifiedTime{ modifyTime }
		{}
		virtual ~IDirEntry()
		{}

		bool IsDirectory()const noexcept{return Flags & EFileFlags::Directory;}
		EFileFlags Flags{EFileFlags::None};
		fs::path Path;
		uint Size{0};
		TimePoint AccessedTime;
		TimePoint CreatedTime;
		TimePoint ModifiedTime;
	};
	typedef sp<const IDirEntry> IDirEntryPtr;
	struct IDrive
	{
		virtual map<string,IDirEntryPtr> Recursive( path path )noexcept(false)=0;
		virtual IDirEntryPtr Get( path path )noexcept(false)=0;
		virtual IDirEntryPtr Save( path path, const vector<char>& bytes, const IDirEntry& dirEntry )noexcept(false)=0;
		virtual IDirEntryPtr CreateFolder( path path, const IDirEntry& dirEntry )=0;
		virtual void Remove( path path )=0;
		virtual void Trash( path path )=0;
		virtual void TrashDisposal( TimePoint latestDate )=0;
		virtual VectorPtr<char> Load( const IDirEntry& dirEntry )=0;
		virtual void Restore( sv name )noexcept(false)=0;
		virtual void SoftLink( path existingFile, path newSymLink )noexcept(false)=0;
	};

#pragma region DiskWatcher
	struct JDE_NATIVE_VISIBILITY DiskWatcher : std::enable_shared_from_this<DiskWatcher>
	{
		DiskWatcher( path path, EDiskWatcherEvents events/*=DefaultEvents*/ )noexcept(false);
		virtual ~DiskWatcher();
		constexpr static EDiskWatcherEvents DefaultEvents = {EDiskWatcherEvents::Modify | EDiskWatcherEvents::MovedFrom | EDiskWatcherEvents::MovedTo | EDiskWatcherEvents::Create | EDiskWatcherEvents::Delete};
	protected:
		virtual void OnModify( path path, const NotifyEvent& /*event*/ )noexcept{WARNN( "No listener for OnModify {}.", path.string() );};
		virtual void OnMovedFrom( path path, const NotifyEvent& /*event*/ )noexcept{WARNN( "No listener for OnMovedFrom {}.", path.string() );};
		virtual void OnMovedTo( path path, const NotifyEvent& /*event*/ )noexcept{WARNN( "No listener for OnMovedTo {}.", path.string() );};
		virtual void OnCreate( path path, const NotifyEvent& /*event*/ )noexcept=0;
		virtual void OnDelete( path path, const NotifyEvent& /*event*/ )noexcept{WARNN( "No listener for OnDelete {}.", path.string() );};
	private:
		void Run()noexcept;
		void ReadEvent( const pollfd& fd, bool isRetry=false )noexcept(false);
		EDiskWatcherEvents _events{DefaultEvents};
		map<uint32_t, fs::path> _descriptors;
		fs::path _path;
 		int _fd;
		sp<Jde::Threading::InterruptibleThread> _pThread;
	};
#pragma endregion
}