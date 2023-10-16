cmake_minimum_required(VERSION 3.16)
project(Jde)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake )
include(SetUpConfigurations)
#######################################################

set( sources )
list( APPEND sources DateTime.cpp Exception.cpp Stopwatch.cpp Str.cpp Cache.cpp Settings.cpp App.cpp Log.cpp
	io/AsioContextThread.cpp io/DiskWatcher.cpp io/File.cpp io/FileCo.cpp io/ProtoClient.cpp io/ServerSink.cpp io/Socket.cpp io/tinyxml2.cpp io/proto/messages.pb.cc
	threading/Interrupt.cpp threading/InterruptibleThread.cpp threading/Pool.cpp threading/Worker.cpp threading/Mutex.cpp
	coroutine/Alarm.cpp coroutine/Coroutine.cpp coroutine/CoWorker.cpp coroutine/Lock.cpp coroutine/Task.cpp
	db/Await.cpp db/Database.cpp db/DataSource.cpp db/DataType.cpp db/DBException.cpp db/DBQueue.cpp db/SchemaProc.cpp db/GraphQL.cpp db/GraphQuery.cpp db/types/Table.cpp
	um/UM.cpp)
if(NOT MSVC)
    list(APPEND sources ../../Linux/source/LinuxApp.cpp ../../Linux/source/LinuxDrive.cpp ../../Linux/source/LinuxTimezone.cpp  ../../Linux/source/LinuxThread.cpp)
endif()
add_library( Jde SHARED ${sources} )
#add_library( Jde.UM SHARED um/UM.cpp )

#######################################################
#include_directories( "../../Public" )
target_link_libraries(Jde)
#######################################################
#######################################################
target_precompile_headers(Jde
  PRIVATE
	<functional>
	<iostream>
	<boost/exception/diagnostic_information.hpp>
	<boost/core/noncopyable.hpp>
	<spdlog/spdlog.h>
	<spdlog/sinks/basic_file_sink.h>

  <jde/TypeDefs.h>
	<jde/Assert.h>
	<jde/Exports.h>
	<jde/Log.h>
	<jde/Exception.h>
	"Settings.h"

	#"log/server/proto/messages.pb.h"
)
