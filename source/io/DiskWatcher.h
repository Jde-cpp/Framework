#pragma once
#include <jde/Exports.h>
#include "../threading/InterruptibleThread.h"
#include "../coroutine/Awaitable.h"
#include <shared_mutex>
struct inotify_event;
struct pollfd;
namespace Jde::IO
{
	using namespace Coroutine;
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
	struct Γ IDriveChange
	{
		β OnAccess( path path, const NotifyEvent& event )noexcept->void;
		β OnModify( path path, const NotifyEvent& event )noexcept->void;
		β OnAttribute( path path, const NotifyEvent& event )noexcept->void;
		β OnCloseWrite( path path, const NotifyEvent& event )noexcept->void;
		β OnCloseNoWrite( path path, const NotifyEvent& event )noexcept->void;
		β OnOpen( path path, const NotifyEvent& event )noexcept->void;
		β OnMovedFrom( path path, const NotifyEvent& event )noexcept->void;
		β OnMovedTo( path path, const NotifyEvent& event )noexcept->void;
		β OnCreate( path path, const NotifyEvent& event )noexcept->void;
		β OnDelete( path path, const NotifyEvent& event )noexcept->void;
		β OnDeleteSelf( path path, const NotifyEvent& event )noexcept->void;
		β OnMoveSelf( path path, const NotifyEvent& event )noexcept->void;
		β OnUnmount( path path, const NotifyEvent& event )noexcept->void;
		β OnQOverflow( path path, const NotifyEvent& event )noexcept->void;
		β OnIgnored( path path, const NotifyEvent& event )noexcept->void;
	private:
		const LogTag& _logLevel{ Logging::TagLevel("driveWatcher") };
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

	struct IDrive : std::enable_shared_from_this<IDrive>
	{
		β Recursive( path path, SRCE )noexcept(false)->flat_map<string,IDirEntryPtr> =0;
		β Get( path path )noexcept(false)->IDirEntryPtr=0;
		β Save( path path, const vector<char>& bytes, const IDirEntry& dirEntry )noexcept(false)->IDirEntryPtr=0;
		β CreateFolder( path path, const IDirEntry& dirEntry )->IDirEntryPtr=0;
		β Remove( path path )->void=0;
		β Trash( path path )->void=0;
		β TrashDisposal( TimePoint latestDate )->void=0;
		β Load( const IDirEntry& dirEntry )->VectorPtr<char> =0;
		β Restore( sv name )noexcept(false)->void=0;
		β SoftLink( path existingFile, path newSymLink )noexcept(false)->void=0;
	};

#pragma region DiskWatcher
	struct Γ DiskWatcher : std::enable_shared_from_this<DiskWatcher>
	{
		DiskWatcher( path path, EDiskWatcherEvents events/*=DefaultEvents*/ )noexcept(false);
		virtual ~DiskWatcher();
		constexpr static EDiskWatcherEvents DefaultEvents = {EDiskWatcherEvents::Modify | EDiskWatcherEvents::MovedFrom | EDiskWatcherEvents::MovedTo | EDiskWatcherEvents::Create | EDiskWatcherEvents::Delete};
	protected:
		β OnModify( path path, const NotifyEvent& /*event*/ )noexcept->void{WARN( "No listener for OnModify {}."sv, path.string() );};
		β OnMovedFrom( path path, const NotifyEvent& /*event*/ )noexcept->void{WARN( "No listener for OnMovedFrom {}."sv, path.string() );};
		β OnMovedTo( path path, const NotifyEvent& /*event*/ )noexcept->void{WARN( "No listener for OnMovedTo {}."sv, path.string() );};
		β OnCreate( path path, const NotifyEvent& /*event*/ )noexcept->void=0;
		β OnDelete( path path, const NotifyEvent& /*event*/ )noexcept->void{WARN( "No listener for OnDelete {}."sv, path.string() );};
	private:
		α Run()noexcept->void;
		α ReadEvent( const pollfd& fd, bool isRetry=false )noexcept(false)->void;
		EDiskWatcherEvents _events{DefaultEvents};
		flat_map<uint32_t, fs::path> _descriptors;
		fs::path _path;
 		int _fd;
		sp<Jde::Threading::InterruptibleThread> _pThread;
	};
#pragma endregion
}