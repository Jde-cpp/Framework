cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_DIR}/Public/build/functions.cmake )
project( Framework VERSION ${JDE_VERSION} LANGUAGES CXX )
include( dependencies.cmake )

link_directories( ${JSONNET_DIR}/lib )

set( targetName Jde )
add_library( ${targetName} SHARED )

set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )

file(GLOB sources *.cpp **/*.cpp **/**/*.cpp )
file(GLOB headers   "**/*.h")
set( newDir $ENV{JDE_DIR}/Public/libs/framework )
set( osDir ${newDir}/process/os )
if(MSVC)
	list(APPEND sources ${osDir}/windows/WindowsApp.cpp ${osDir}/windows/WindowsDrive.cpp ${osDir}/windows/WindowsSvc.cpp  ${osDir}/windows/WindowsThread.cpp ${osDir}/windows/WindowsUtilities.cpp  ${osDir}/windows/WindowsWorker.cpp)
else()
  list(APPEND sources ${osDir}/linux/LinuxApp.cpp ${osDir}/linux/LinuxDrive.cpp ${osDir}/linux/LinuxTimezone.cpp  ${osDir}/linux/LinuxThread.cpp)
endif()
get_filename_component(thread_cpp ${CMAKE_CURRENT_SOURCE_DIR}/threading/Thread.cpp ABSOLUTE)
list(REMOVE_ITEM sources "${thread_cpp}")
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} fmt::fmt )
target_link_libraries( ${targetName} jsonnet++ jsonnet )

target_precompile_headers( ${targetName}
  PUBLIC
  <jde/framework.h>
)