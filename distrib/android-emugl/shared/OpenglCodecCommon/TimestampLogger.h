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

#pragma once

#include <stdint.h>
#include <stdlib.h>

class TimestampLogger {
public:
  TimestampLogger() : m_timestampEnabled(false) {}
  ~TimestampLogger() {}

  TimestampLogger(TimestampLogger const&) = delete;
  void operator=(TimestampLogger const&) = delete;

  uint32_t getTimestampSize();

  bool getTimestampEnabled();
  void setTimestampEnabled(const bool timestampEnabled);

  uint64_t getTimestamp();

  void logTimestamp(const void* tstamp, const size_t tstampSize);

private:
  bool m_timestampEnabled;
};
