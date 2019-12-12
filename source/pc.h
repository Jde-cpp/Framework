#ifdef _MSC_VER
//	#pragma once
	#include <SDKDDKVer.h>
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif
//libmatio.lib;
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/SVD>

#pragma warning(push)
#pragma warning( disable : 4245) 
#pragma warning( disable : 4701)
#include <boost/crc.hpp>
#pragma warning(pop)

#include <boost/exception/diagnostic_information.hpp> 
#include <boost/asio.hpp>
#include <boost/asio/ts/io_context.hpp>
#include <boost/core/noncopyable.hpp>


//#include <boost/uuid/uuid.hpp>
//#include <boost/uuid/uuid_generators.hpp>
//#include <boost/uuid/uuid_io.hpp>

#include <nlohmann/json.hpp>

#ifndef __INTELLISENSE__
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/basic_file_sink.h>
	#include <spdlog/fmt/ostr.h>
#endif

#include "log/Logging.h"
#include "Exception.h"
#include "JdeAssert.h"
#include "TypeDefs.h"

#pragma warning(push)
#pragma warning( disable : 4100 )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )
#include "google/protobuf/message.h"
#include "log/server/proto/messages.pb.h"
#pragma warning(pop)

/*
#ifdef _WINDOWS
	#ifndef __INTELLISENSE__
		#include <windows.h>
	#endif
#endif
*/