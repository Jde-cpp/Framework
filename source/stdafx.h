/*
#ifdef _WINDLL
#ifndef WINVER
	#define WINVER 0x0A00
	#define _WIN32_WINNT 0x0A00
#endif
	#include <SDKDDKVer.h>
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif
*/
#ifdef _MSC_VER
	#define WIN32_LEAN_AND_MEAN
	//#include <Windows.h>
	//#include <winsock2.h>
#endif

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

#pragma warning(push)
#pragma warning( disable : 4245) 
#pragma warning( disable : 4701)
#include <boost/crc.hpp>
#pragma warning(pop)

#include <boost/exception/diagnostic_information.hpp> 
//#include <boost/asio.hpp>
//#include <boost/asio/ts/io_context.hpp>
#include <boost/core/noncopyable.hpp>


//#include <boost/uuid/uuid.hpp>
//#include <boost/uuid/uuid_generators.hpp>
//#include <boost/uuid/uuid_io.hpp>

//#include <nlohmann/json.hpp>  //writer.cpp:5695: sorry: not yet implemented

#ifndef __INTELLISENSE__
#ifdef MSC_VER
//	#include <format>
	#include <fmt/core.h>
#endif
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/basic_file_sink.h>
	#include <spdlog/fmt/ostr.h>
#endif

#include "log/Logging.h"
//#include "Exception.h"
#include "JdeAssert.h"
#include "Exports.h"
#include "TypeDefs.h"

/*
#pragma warning(push)
#pragma warning( disable : 4100 )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )
#pragma warning( disable : 5054 )
#include "google/protobuf/message.h"
#include "log/server/proto/messages.pb.h"
#pragma warning(pop)
*/