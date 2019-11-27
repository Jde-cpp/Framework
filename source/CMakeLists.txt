#export CXX=g++-8
project(Jde)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake )
include(SetUpConfigurations)
#######################################################

add_library( JdeSsl SHARED Ssl.cpp )

#######################################################
#find_program(CCACHE_FOUND ccache)
#if(CCACHE_FOUND)
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
#endif(CCACHE_FOUND)
#######################################################
#set(THREADS_PREFER_PTHREAD_FLAG ON)
#find_package(Threads REQUIRED)
target_link_libraries(JdeSsl Threads::Threads)
#######################################################
#set(Boost_USE_STATIC_LIBS OFF) 
#set(Boost_USE_MULTITHREADED ON)  
#set(Boost_USE_STATIC_RUNTIME OFF) 
#find_package(Boost 1.71.0 COMPONENTS)
#if(Boost_FOUND)
#    include_directories( ${Boost_INCLUDE_DIRS} )
#endif()
#######################################################
include_directories(  "/home/duffyj/code/libraries/spdlog/include" )
include_directories( "/home/duffyj/code/libraries/json/include" )
#set (CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")
#add_link_options( "LINKER:-rpath='$ORIGIN'" )
#######################################################
target_precompile_headers(JdeSsl
  PUBLIC
#    project_header.h
#    "$<$<COMPILE_LANGUAGE:CXX>:cxx_only.h>"
  PRIVATE
		<boost/crc.hpp> 
		<boost/beast/core.hpp>
		<boost/beast/http.hpp>
		<boost/asio/ssl.hpp>
		<boost/beast/version.hpp>
		<boost/asio/ip/tcp.hpp>
		<boost/asio/connect.hpp>
		<boost/system/error_code.hpp>
		<spdlog/spdlog.h>
		<spdlog/sinks/basic_file_sink.h>
		<spdlog/fmt/ostr.h>
		<nlohmann/json.hpp>
		../../Framework/source/DateTime.h
		../../Framework/source/Exception.h
		../../Framework/source/JdeAssert.h
		../../Framework/source/log/Logging.h
		../../Framework/source/TypeDefs.h
		../../Framework/source/io/File.h)
