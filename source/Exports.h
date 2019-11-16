#pragma once
#ifdef JDE_NATIVE_EXPORTS
	#ifdef _MSC_VER 
		#define JDE_NATIVE_VISIBILITY __declspec( dllexport )
	#else
		#define JDE_NATIVE_VISIBILITY __attribute__((visibility("default")))
	#endif
#else 
	#ifdef _MSC_VER
		#define JDE_NATIVE_VISIBILITY __declspec( dllimport )
		//#define _GLIBCXX_USE_NOEXCEPT noexcept
		#if NDEBUG
			#pragma comment(lib, "Jde.Native.lib")
		#else
			#pragma comment(lib, "Jde.Native.lib")
		#endif
	#else
		#define JDE_NATIVE_VISIBILITY
	#endif
#endif