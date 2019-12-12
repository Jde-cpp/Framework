#pragma once
#ifndef _MSC_VER
#ifndef __INTELLISENSE__
	#include <dlfcn.h>
#endif
	namespace Jde
	{
		typedef void* HMODULE;
		typedef void* FARPROC;
	}
#endif

namespace Jde
{
	//https://blog.benoitblanchon.fr/getprocaddress-like-a-boss/
	struct ProcPtr 
	{
		ProcPtr( FARPROC ptr ): 
			_ptr(ptr)
		{}

		template <typename T, typename = std::enable_if_t<std::is_function_v<T>>>
		operator T *() const 
		{
			return reinterpret_cast<T *>(_ptr);
		}
	private:
		FARPROC _ptr;
	};
	struct DllHelper
	{
		DllHelper( const fs::path& path ): 
			_path{path},
#if _MSC_VER
			_module{ ::LoadLibrary(path.string().c_str()) }
#else
			_module{ ::dlopen( path.c_str(), RTLD_LAZY ) }
#endif
		{
			if( !_module )
#ifdef _MSC_VER
				THROW( IOException("Can not load library '{}' - '{:x}'", path.string(), GetLastError()) );
#else
				THROW( IOException("Can not load library '{}':  '{}'", path.c_str(), dlerror()) );
#endif
			INFON( "Opened module '{}'.", path.string() );
		}
		~DllHelper()
		{
			if( GetDefaultLogger() )
				DBGN( "Freeing '{}'.", _path.string() );
#if _MSC_VER			
			::FreeLibrary( _module ); 
#else
			::dlclose( _module );
#endif
			if( GetDefaultLogger() )
				DBG( "Freed '{}'.", _path.string() );
		}
		

		ProcPtr operator[](string_view proc_name) const 
		{
#if _MSC_VER
			auto procAddress = ::GetProcAddress( _module, string(proc_name).c_str() );
#else
			auto procAddress = ::dlsym( _module, string(proc_name).c_str() );
#endif
			ASSRT_NN( procAddress );
			return ProcPtr( procAddress );
		}
		//static HMODULE _parent_module;
	private:
		fs::path _path;
		HMODULE _module;
	};
}