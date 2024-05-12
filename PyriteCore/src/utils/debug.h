#pragma once

#include <string>
#include <stdexcept>
#include "logger.h"

#include "Logger.h"

PYR_DECLARELOG(LogDebug);

#ifdef _DEBUG
#define PYR_ISDEBUG
#define PYR_DEBUG(x) x
#define PYR_ASSERT(x, ...) if(!(x)) throw std::runtime_error(std::string("Broken assertion: ") + Logger::concat(__VA_ARGS__))
#define PYR_ENSURE(x, ...)\
  ([&]{\
    static bool _triggered = false;\
    bool valid = !!(x);\
    if (!valid && !_triggered) {\
      PYR_LOG(LogDebug, WARN, "Ensure failed at " __FILE__ ":", __LINE__, " ", __VA_ARGS__);\
      _triggered = true;\
    }\
    return valid;\
  }())
#else
#define PYR_DEBUG(x)
#define PYR_ASSERT(x, ...)
#define PYR_ENSURE(x, ...) !!(x)
#endif