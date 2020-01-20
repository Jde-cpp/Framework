#export CXX=g++-8
cmake_minimum_required(VERSION 3.16)
project(Jde)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake )
include(SetUpConfigurations)
#######################################################

set( sources )
list( APPEND sources DateTime.cpp Diagnostics.cpp Exception.cpp Stopwatch.cpp StringUtilities.cpp Cache.cpp Settings.cpp
	math/MathUtilities.cpp
	application/Application.cpp
	log/Logging.cpp log/server/ServerSink.cpp log/server/proto/messages.pb.cc
	io/sockets/Socket.cpp io/sockets/ProtoClient.cpp io/sockets/ProtoServer.cpp
	io/File.cpp io/DiskWatcher.cpp
	threading/Thread.cpp threading/Interrupt.cpp threading/InterruptibleThread.cpp threading/Pool.cpp
	db/DBException.cpp db/DataType.cpp db/Database.cpp db/DBQueue.cpp)
if(NOT MSVC)
    list(APPEND sources ../../Linux/source/LinuxApp.cpp ../../Linux/source/LinuxDrive.cpp ../../Linux/source/LinuxTimezone.cpp)
endif()
add_library( Jde SHARED ${sources} )

#######################################################
include_directories( "$ENV{REPO_DIR}/eigen" )
target_link_libraries(Jde Threads::Threads)
#######################################################
target_precompile_headers(Jde
  PRIVATE
	<functional>
	<list>
	<map>
	<memory>
	<mutex>
	<set>
	<shared_mutex>
	<string>
	<vector>
	<iostream>
	<filesystem>
	<Eigen/Dense>
	<Eigen/Sparse>
	<Eigen/SVD>
	<boost/crc.hpp>
	<boost/exception/diagnostic_information.hpp>
	<boost/asio.hpp>
	<boost/asio/ts/io_context.hpp>
	<boost/core/noncopyable.hpp>
	<nlohmann/json.hpp>
	<spdlog/spdlog.h>
	<spdlog/sinks/basic_file_sink.h>
	<spdlog/fmt/ostr.h>
	<google/protobuf/message.h>
	"log/Logging.h"
	"Exception.h"
	"JdeAssert.h"
	"TypeDefs.h"
	"log/server/proto/messages.pb.h"
)
