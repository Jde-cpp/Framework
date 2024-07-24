cmake_minimum_required(VERSION 3.16)
project(Jde)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
link_directories( ${fmt_DIR}/../.. )
link_directories( ${Protobuf_DIR}/../.. )
#######################################################
add_library( ${CMAKE_PROJECT_NAME} SHARED )

file(GLOB src_files *.cpp **/*.cpp **/**/*.cpp )
file(GLOB hdr_files   "**/*.h")
if(NOT MSVC)
    list(APPEND src_files ../../Linux/source/LinuxApp.cpp ../../Linux/source/LinuxDrive.cpp ../../Linux/source/LinuxTimezone.cpp  ../../Linux/source/LinuxThread.cpp)
		get_filename_component(thread_cpp ${CMAKE_CURRENT_SOURCE_DIR}/threading/Thread.cpp ABSOLUTE)
		list(REMOVE_ITEM src_files "${thread_cpp}")
endif()

target_sources("${CMAKE_PROJECT_NAME}"
    PRIVATE
        "${src_files}"
        "${hdr_files}"
)


#######################################################
target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} libprotobuf.a libutf8_range.a libutf8_validity.a )
#target_link_libraries( ${CMAKE_PROJECT_NAME} absl::cord absl::log_internal_check_op absl::log_internal_message absl::strings )
#######################################################
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
	<functional>
	<iostream>
	<boost/exception/diagnostic_information.hpp>
	<boost/core/noncopyable.hpp>
#	<spdlog/spdlog.h>
#	<spdlog/sinks/basic_file_sink.h>

  <jde/TypeDefs.h>
	<jde/Assert.h>
	<jde/Exports.h>
	<jde/log/Log.h>
	<jde/Exception.h>
	"Settings.h"

	#"log/server/proto/messages.pb.h"
)
