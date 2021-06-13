#pragma warning( push, 0  )
#include <jde/TypeDefs.h>

#include <functional>
#include <iostream>

#include <boost/crc.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/core/noncopyable.hpp>

#ifndef __INTELLISENSE__
#ifdef MSC_VER
	#include <fmt/core.h>
#endif
#pragma warning( disable: 4702  )
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/basic_file_sink.h>
	#include <spdlog/fmt/ostr.h>
#endif
#pragma warning( pop  )

#include <jde/Log.h>
#include <jde/Assert.h>
#include <jde/Exports.h>

