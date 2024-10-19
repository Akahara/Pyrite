#pragma once

#include <string>
#include <stdexcept>
#include "logger.h"

#include "Logger.h"

PYR_DECLARELOG(LogDebug);

#ifdef _DEBUG
#define PYR_ISDEBUG
#define PYR_DEBUG(x) x
#define PYR_ASSERT(x, ...)\
  if(!(x)) {\
    PYR_LOG(LogDebug, FATAL, "Broken assertion: ", __VA_ARGS__);\
    throw std::runtime_error(std::string("Broken assertion: ") + Logger::concat(__VA_ARGS__));\
  }
#define PYR_ENSURE(x, ...)\
  ([&]{\
    static bool _triggered = false;\
    bool _valid = !!(x);\
    if (!_valid && !_triggered) {\
      PYR_LOG(LogDebug, WARN, "Ensure failed at " __FILE__ ":", __LINE__, " ", __VA_ARGS__);\
      _triggered = true;\
      __debugbreak();\
    }\
    return _valid;\
  }())

#else
#define PYR_DEBUG(x)
#define PYR_ASSERT(x, ...)
#define PYR_ENSURE(x, ...) !!(x)
#endif