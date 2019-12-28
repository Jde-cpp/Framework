#pragma once
//#include <forward_list>
//#include "threading/InterruptibleThread.h"
#include "../Exports.h"
namespace Jde::Threading{ struct InterruptibleThread; }

namespace Jde
{
	struct IShutdown
	{
		virtual void Shutdown()noexcept=0;
	};

	struct JDE_NATIVE_VISIBILITY IApplication
	{
		virtual ~IApplication();
		void BaseStartup( int argc, char** argv, string_view appName )noexcept;

		static void AddThread( sp<Threading::InterruptibleThread> pThread )noexcept;
		static void RemoveThread( sp<Threading::InterruptibleThread> pThread )noexcept;
		static void GarbageCollect()noexcept;
		static void AddApplicationLog( ELogLevel level, const string& value )noexcept;//static to call in std::terminate.
		static void AddShutdown( sp<IShutdown> pShared )noexcept;
		static void Add( sp<void> pShared )noexcept;
		static bool Kill( uint processId )noexcept{return _pInstance ? _pInstance->KillInstance( processId ) : false;}
		static void Remove( sp<void> pShared )noexcept;
		static void CleanUp()noexcept;
		static TimePoint StartTime()noexcept;
		static void AddShutdownFunction( std::function<void()>&& shutdown )noexcept;
		static void Pause()noexcept;
		static std::list<sp<Threading::InterruptibleThread>>& GetBackgroundThreads()noexcept{ return  *_pBackgroundThreads; }
	protected:
		void Wait()noexcept;
		static void OnTerminate()noexcept;//implement in OSApp.cpp.
		virtual void OSPause()noexcept=0;
		virtual bool AsService()noexcept=0;
		virtual void AddSignals()noexcept(false)=0;
		virtual bool KillInstance( uint processId )noexcept=0;

		static mutex _threadMutex;
		static sp<std::list<sp<Threading::InterruptibleThread>>> _pBackgroundThreads;

		static sp<IApplication> _pInstance;
	private:
		virtual void SetConsoleTitle( string_view title )noexcept=0;
	};

	struct OSApp : IApplication
	{
		JDE_NATIVE_VISIBILITY static void Startup( int argc, char** argv, string_view appName )noexcept;
	protected:
		bool KillInstance( uint processId )noexcept override;
		void SetConsoleTitle( string_view title )noexcept override;
		void AddSignals()noexcept(false) override;
		bool AsService()noexcept override;
		void OSPause()noexcept override;
		//void OnTerminate()noexcept override;
	private:
		static void ExitHandler( int s );
#ifdef _MSC_VER
		BOOL HandlerRoutine( DWORD  ctrlType );
#endif
	};

namespace Application
{
	//JDE_NATIVE_VISIBILITY void Startup( int argc, char** argv, string_view appName )noexcept;
	//JDE_NATIVE_VISIBILITY void Pause()noexcept;


	//JDE_NATIVE_VISIBILITY

/*		template<class Function, class... Args>
	static int Main( bool checkForDone, Function&& f, Args&&... args )
	{
		/ *auto pLogger = Jde::GetDefaultLogger();
		Jde::Threading::SetThreadDescription( "main" );
		pLogger->set_level( spdlog::level::debug );
		pLogger->info( "Start" );* /

		BackgroundThreads.push_front( Threading::InterruptibleThread(f, args...) );
		WaitHandler( checkForDone );
//			WaitHandler( std::function<void()> onExit );
		return 0;
	}
		*/
}}