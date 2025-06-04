boost()

include_directories( $ENV{JDE_DIR}/Public/include )

find_package( fmt REQUIRED )

find_package( spdlog REQUIRED )
include_directories( ${spdlog_DIR}/../../../include ) #${spdlog_INCLUDE_DIRS}

if( WIN32 )
	include_directories( ${JSONNET_ROOT}/include )
	link_directories( ${JSONNET_LIB_DIR} )
else()
	if( ${LIB_DIR} STREQUAL "" )
		message( FATAL_ERROR "Set LIB_DIR environment variable" )
	endif()
	set( JSONNET_DIR ${LIB_DIR}/jsonnet CACHE PATH "Jsonnet directory" )
	include_directories( ${JSONNET_DIR}/include )
	link_directories( ${JSONNET_DIR}/lib )
endif()
