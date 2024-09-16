cmake_minimum_required(VERSION 3.20.0)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET(CMAKE_TOOLCHAIN_FILE "$ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"  CACHE STRING "Vcpkg toolchain file")
endif()
project(Jde)

include(compiler)

if(MSVC)
#	find_package(Protobuf CONFIG REQUIRED)
	find_package(fmt)
else()
	link_directories( ${fmt_DIR}/../.. )
	link_directories( ${Protobuf_DIR}/../.. )
endif()
#######################################################
add_library( ${CMAKE_PROJECT_NAME} SHARED )

file(GLOB sources *.cpp **/*.cpp **/**/*.cpp )
file(GLOB headers   "**/*.h")
if(MSVC)
	list(APPEND sources ../../Windows/source/WindowsApp.cpp ../../Windows/source/WindowsDrive.cpp ../../Windows/source/WindowsSvc.cpp  ../../Windows/source/WindowsThread.cpp ../../Windows/source/WindowsUtilities.cpp  ../../Windows/source/WindowsWorker.cpp)
else()
  list(APPEND sources ../../Linux/source/LinuxApp.cpp ../../Linux/source/LinuxDrive.cpp ../../Linux/source/LinuxTimezone.cpp  ../../Linux/source/LinuxThread.cpp)
endif()
get_filename_component(thread_cpp ${CMAKE_CURRENT_SOURCE_DIR}/threading/Thread.cpp ABSOLUTE)
list(REMOVE_ITEM sources "${thread_cpp}")

target_sources( ${CMAKE_PROJECT_NAME} PRIVATE ${sources} ${headers} )


#######################################################
if(MSVC)
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
	target_link_libraries( ${CMAKE_PROJECT_NAME} Version.lib )
else()
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} libprotobuf.a libutf8_range.a libutf8_validity.a )
endif()
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
	<functional>
	<iostream>
#	<boost/exception/diagnostic_information.hpp>
	<boost/core/noncopyable.hpp>

  <jde/TypeDefs.h>
	<jde/Assert.h>
	<jde/Exports.h>
	<jde/log/Log.h>
	<jde/Exception.h>
	Settings.h
)
if( MSVC )
	set( CMAKE_INSTALL_PREFIX $ENV{JDE_DIR}/installed )
	set( installDir ${CMAKE_INSTALL_PREFIX}/$<CONFIGURATION> )
	set( configStem jdeConfig )
	install( TARGETS ${CMAKE_PROJECT_NAME}
		EXPORT ${configStem}
		RUNTIME DESTINATION ${installDir}/bin
		ARCHIVE DESTINATION ${installDir}/lib )
	install( FILES $<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}> CONFIGURATIONS Debug RelWithDebInfo DESTINATION ${installDir}/bin )
#/home/duffyj/code/libraries/install/g++-13/asan/spdlog/lib/cmake/spdlog/
	export( TARGETS ${CMAKE_PROJECT_NAME} NAMESPACE Jde:: FILE ${CMAKE_CURRENT_BINARY_DIR}/${configStem}.cmake )
	install( EXPORT ${configStem} DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake NAMESPACE Jde:: )
	#install( FILES "${CMAKE_CURRENT_BINARY_DIR}/${exportFile}.cmake" DESTINATION ${installDir}/lib/cmake/${CMAKE_PROJECT_NAME} )
endif()