#include "clock.h"

#include <Windows.h>

namespace pyr
{

PerformanceClock::PerformanceClock() {
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  m_secondsPerCount = 1. / static_cast<double>(frequency.QuadPart);
}

int64_t PerformanceClock::getTimeAsCount() const {
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return counter.QuadPart;
}

double PerformanceClock::getDeltaSeconds(int64_t from, int64_t to) const
{
  return static_cast<double>(to - from) * m_secondsPerCount;
}

}
