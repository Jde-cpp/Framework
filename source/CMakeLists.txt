cmake_minimum_required(VERSION 3.16)
project(Jde)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
link_directories( ${fmt_DIR}/../.. )
link_directories( ${Protobuf_DIR}/../.. )
#######################################################

set( sources )
list( APPEND sources DateTime.cpp Exception.cpp Stopwatch.cpp Str.cpp Cache.cpp Settings.cpp App.cpp Log.cpp
	io/AsioContextThread.cpp io/DiskWatcher.cpp io/File.cpp io/FileCo.cpp io/ProtoClient.cpp io/ServerSink.cpp io/Socket.cpp io/tinyxml2.cpp io/proto/messages.pb.cc
	threading/Interrupt.cpp threading/InterruptibleThread.cpp threading/Pool.cpp threading/Worker.cpp threading/Mutex.cpp
	coroutine/Alarm.cpp coroutine/Coroutine.cpp coroutine/CoWorker.cpp coroutine/Lock.cpp coroutine/Task.cpp
	db/Await.cpp db/Database.cpp db/DataSource.cpp db/DataType.cpp db/DBException.cpp db/DBQueue.cpp db/SchemaProc.cpp db/GraphQL.cpp db/GraphQuery.cpp db/types/Table.cpp
	db/graphQL/Introspection.cpp
	um/UM.cpp)
if(NOT MSVC)
    list(APPEND sources ../../Linux/source/LinuxApp.cpp ../../Linux/source/LinuxDrive.cpp ../../Linux/source/LinuxTimezone.cpp  ../../Linux/source/LinuxThread.cpp)
endif()
add_library( ${CMAKE_PROJECT_NAME} SHARED ${sources} )

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
	<jde/Log.h>
	<jde/Exception.h>
	"Settings.h"

	#"log/server/proto/messages.pb.h"
)
