boost()

include_directories( $ENV{JDE_DIR}/Public/include )

find_package( fmt REQUIRED )

find_package( spdlog REQUIRED )
include_directories( ${spdlog_DIR}/../../../include ) #${spdlog_INCLUDE_DIRS}

if( WIN32 )
	include_directories( ${JSONNET_ROOT}/include )
	link_directories( ${JSONNET_LIB_DIR} )
	#get_filename_component(BOOST_LIB_DIR ${Boost_JSON_LIBRARY_DEBUG} DIRECTORY)
	#link_directories( ${Boost_LIBRARY_DIR} )
else()
#	set( JSONNET_DIR $ENV{LIB_DIR}/jsonnet CACHE PATH "Jsonnet directory" )
#	include_directories( ${JSONNET_ROOT}/include )
endif()
