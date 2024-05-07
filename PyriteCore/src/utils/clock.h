#pragma once

#include <stdint.h>

namespace pyr
{

class PerformanceClock {
public:
  PerformanceClock();

  int64_t getTimeAsCount() const;
  double getSecondsPerCount() const { return m_secondsPerCount; }
  double getDeltaSeconds(int64_t from, int64_t to) const;
private:
  double m_secondsPerCount;
};

}