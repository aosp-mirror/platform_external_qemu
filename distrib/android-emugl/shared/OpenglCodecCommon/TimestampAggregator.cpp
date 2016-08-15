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

#include "TimestampAggregator.h"
#include "TimestampPacketManip.h"

#include <cmath>
#include <ctime>
#include <iostream>

using std::cout;

TimestampAggregator::~TimestampAggregator() {
  // Log stats upon death of RenderControl Thread
  cout << "Timestamp Logs:\n\t Min: " << stats.getMin() << "\n\t Max: "
       << stats.getMax() << "\n\t Average: " << stats.getMean()
       << "\n\t Variance: " << stats.getVariance() << "\n";
}

size_t TimestampAggregator::getTimestampSize() {
  return timestampEnabled ? TimestampPacketManip::getTimestampSize() : 0;
}

void TimestampAggregator::setTimestampEnabled(const bool timestampEnabled_) {
  timestampEnabled = timestampEnabled_;
}

void TimestampAggregator::logTimestamp(void* ptr) {
  if (!timestampEnabled) {
    return;
  }

  uint64_t old_tstamp = TimestampPacketManip::stripPacket(ptr);
  if (!old_tstamp) {
    return; // TODO: Decide what to do with fail states
  }

  uint64_t new_tstamp = getTimestamp();
  if (!new_tstamp) {
    return; // TODO: Decide what to do with fail states
  }

  stats.push(new_tstamp - old_tstamp);
}

uint64_t TimestampAggregator::getTimestamp() {
  constexpr long sec_to_nsec = pow(10, 9);
  timespec time;

  if (clock_gettime(CLOCK_REALTIME, &time) < 0) { // TODO: Ping correct clock
    return 0;
  }

  return static_cast<uint64_t>(time.tv_sec) * sec_to_nsec + time.tv_nsec;
}
