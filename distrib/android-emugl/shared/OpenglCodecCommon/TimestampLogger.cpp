/*
* Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "TimestampLogger.h"

#include <cmath>
#include <ctime>
#include <iostream>
#include <cstring>

uint32_t TimestampLogger::getTimestampSize() {
  return sizeof(uint64_t);
}

bool TimestampLogger::getTimestampEnabled() {
  return m_timestampEnabled;
}

void TimestampLogger::setTimestampEnabled(const bool timestampEnabled) {
  std::cout << "LUUUUUUUM" << "\n";
  m_timestampEnabled = timestampEnabled;
}

uint64_t TimestampLogger::getTimestamp() {
  constexpr long sec_to_nsec = pow(10, 9);
  timespec time;

  if (clock_gettime(CLOCK_REALTIME, &time) < 0) {
    return 0;
  }

  return static_cast<uint64_t>(time.tv_sec) * sec_to_nsec + time.tv_nsec;
}

void TimestampLogger::logTimestamp(const void* tstamp,
                                   const size_t tstampSize)
{
  uint64_t old_tstamp;
  memcpy(&old_tstamp, tstamp, tstampSize);
  if (!old_tstamp) {
    return; // TODO: fail silently?
  }

  uint64_t new_tstamp = getTimestamp();
  if (!new_tstamp) {
    return; // TODO: fail silently?
  }

  // TODO: Logggggggggggggg it... right
  std::cout << "LUUM LOG: Timestamp Diff: " << new_tstamp - old_tstamp << "\n";
}
