#include <jde/TypeDefs.h>

#ifdef _MSC_VER
	#include <memoryapi.h>
#endif

#include <functional>
#include <iostream>
#include <thread>

#include <boost/system/error_code.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/core/noncopyable.hpp>

#include <jde/Exports.h>
#include <jde/log/Log.h>
#include <jde/Exception.h>
#include <jde/Assert.h>
#include "Settings.h"