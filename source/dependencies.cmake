boost()

include_directories( $ENV{JDE_DIR}/Public/include )

find_package( fmt REQUIRED )

find_package( spdlog REQUIRED )
include_directories( ${spdlog_DIR}/../../../include ) #${spdlog_INCLUDE_DIRS}

set( JSONNET_DIR $ENV{LIB_DIR}/jsonnet CACHE PATH "Jsonnet directory" )
include_directories( ${JSONNET_DIR}/include )

