#pragma once
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
		static IApplication& Instance(){ ASSERT(_pInstance); return *_pInstance; }
		set<string> BaseStartup( int argc, char** argv, string_view appName, string_view companyName="jde-cpp" )noexcept(false);

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
		static vector<sp<Threading::InterruptibleThread>>& GetBackgroundThreads()noexcept{ return  *_pBackgroundThreads; }
		static string_view CompanyName()noexcept{ return _pCompanyName ? *_pCompanyName : ""sv;}
		static string_view ApplicationName()noexcept{ return _pApplicationName ? *_pApplicationName : ""sv;}
		virtual fs::path ProgramDataFolder()noexcept=0;
		virtual fs::path ApplicationDataFolder()noexcept{ return ProgramDataFolder()/CompanyName()/ApplicationName(); }


	protected:
		void Wait()noexcept;
		static void OnTerminate()noexcept;//implement in OSApp.cpp.
		virtual void OSPause()noexcept=0;
		virtual bool AsService()noexcept=0;
		virtual void AddSignals()noexcept(false)=0;
		virtual bool KillInstance( uint processId )noexcept=0;
		virtual string GetEnvironmentVariable( string_view variable )noexcept=0;

		static mutex _threadMutex;
		static VectorPtr<sp<Threading::InterruptibleThread>> _pBackgroundThreads;

		static sp<IApplication> _pInstance;
		static unique_ptr<string> _pApplicationName;
		static unique_ptr<string> _pCompanyName;
	private:
		virtual void SetConsoleTitle( string_view title )noexcept=0;
	};

	struct OSApp : IApplication
	{
		JDE_NATIVE_VISIBILITY static set<string> Startup( int argc, char** argv, string_view appName )noexcept(false);
		string GetEnvironmentVariable( string_view variable )noexcept override;
		fs::path ProgramDataFolder()noexcept override;
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
}