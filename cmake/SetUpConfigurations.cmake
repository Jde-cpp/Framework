#https://stackoverflow.com/questions/31546278/where-to-set-cmake-configuration-types-in-a-project-with-subprojects
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT SET_UP_CONFIGURATIONS_DONE)
	set(SET_UP_CONFIGURATIONS_DONE 1)
	if(CMAKE_CONFIGURATION_TYPES) # multiconfig generator?
		if(MSVC)
			set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
		else()
			set(CMAKE_CONFIGURATION_TYPES "Debug;ASAN;Release;RelWithDebInfo" CACHE STRING "" FORCE)
		endif()
	else()
		if(NOT CMAKE_BUILD_TYPE)
			if(MSVC)
				message( "Defaulting to Debug build." )
				set( CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE )
			else()
				message( "Defaulting to ASAN build." )
				set( CMAKE_BUILD_TYPE ASAN CACHE STRING "" FORCE )
			endif()
		endif()
		set_property( CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build" )
		set_property( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Asan;Debug;Release;Profile" )
	endif()
endif()
#######################################################
string(APPEND RELEASE " -O3 -march=native -Wno-range-loop-construct -Wno-empty-body -Wno-return-type-c-linkage -Wno-implicitly-unsigned-literal -Wno-deprecated-builtins -Wno-c++11-narrowing")
if(MSVC)
	string(APPEND DEBUG " /W4 /wd\"4068\" /wd\"4251\" /wd\"4275\" /wd\"4297\"")
	add_definitions( -DBOOST_ALL_DYN_LINK )
else()
	string(APPEND DEBUG " -O0 -g -D_GLIBCXX_DEBUG")
	string(APPEND DEBUG " -Wall -Wno-range-loop-construct -Wno-unknown-pragmas -Wno-empty-body -Wno-unqualified-std-cast-call -Wno-return-type-c-linkage -Wno-exceptions -Wno-#pragma-messages -Wno-deprecated-builtins -Wno-c++11-narrowing" )
endif()

string(APPEND CMAKE_CXX_FLAGS_RELEASE ${RELEASE} )
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO ${RELEASE} )
string(APPEND CMAKE_CXX_FLAGS_DEBUG ${DEBUG})
#string(APPEND CMAKE_CXX_FLAGS_ASAN )
string(APPEND CMAKE_CXX_FLAGS_ASAN "${DEBUG} -fsanitize=address -fno-omit-frame-pointer -fno-limit-debug-info")
if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
	string(APPEND CMAKE_CXX_FLAGS_ASAN "${DEBUG} -fno-limit-debug-info")
endif()

set(CMAKE_CXX_FLAGS "-std=c++20")
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.78.0 COMPONENTS)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
	#message( "Boost_INCLUDE_DIRS = ${Boost_INCLUDE_DIRS}" )
	#message( "Boost_LIBRARY_DIR = ${Boost_LIBRARY_DIRS}" )
	#message( "BOOST_LIBRARYDIR = ${BOOST_LIBRARYDIR}" )
	#if(MSVC)
		link_directories( ${Boost_LIBRARY_DIRS} )
	#endif()
endif()
#######################################################
if(MSVC)
	include_directories( $ENV{INCLUDE} )
else()
	include_directories( "$ENV{REPO_DIR}/spdlog/include" )
	include_directories( "$ENV{REPO_DIR}/json/include" )
	include_directories( "$ENV{JDE_DIR}/Public" )
	include_directories( "$ENV{REPO_DIR}/protobuf/src" )
	message( "$ENV{REPO_DIR}/protobuf/abseil-cpp" )
	include_directories( "$ENV{REPO_DIR}/protobuf/third_party/abseil-cpp" )
	if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
		include_directories( "$ENV{REPO_DIR}/fmt/include" )
	endif()
endif()
if(MSVC)
	set( outDir "" )
else()
	if( NOT CMAKE_BUILD_TYPE MATCHES RelWithDebInfo )
		string(TOLOWER ${CMAKE_BUILD_TYPE} outDir)
	else()
		set( outDir ${CMAKE_BUILD_TYPE} )
	endif()
	if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++" )
	endif()
endif()
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY $ENV{JDE_DIR}/bin/${outDir} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{JDE_DIR}/bin/${outDir} )
if(NOT MSVC)
	set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,-rpath=$ORIGIN")
endif()
