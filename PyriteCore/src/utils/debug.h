#pragma once

#include <string>
#include <stdexcept>
#include "logger.h"

#ifdef _DEBUG
#define PYR_ISDEBUG
#define PYR_DEBUG(x) x
#define PYR_ASSERT(x, message) if(!(x)) throw std::runtime_error(std::string("Broken assertion: ") + (message))
#else
#define PYR_DEBUG(x)
#define PYR_ASSERT(x, message)
#endif