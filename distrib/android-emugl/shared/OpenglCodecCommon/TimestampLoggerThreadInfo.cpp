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

#include "TimestampLoggerThreadInfo.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/thread_store.h"

namespace {

class TimestampLoggerThreadStore : public ::emugl::ThreadStore {
public:
 TimestampLoggerThreadStore() : ::emugl::ThreadStore(&destructor) {}

private:
 static void destructor(void* value) {
   delete static_cast<TimestampLoggerThreadInfo*>(value);
 }
};

static ::emugl::LazyInstance<TimestampLoggerThreadStore> s_tls =
        LAZY_INSTANCE_INIT;

TimestampLoggerThreadInfo* getTimestampLoggerThreadInfo() {
  TimestampLoggerThreadInfo* pti =
      static_cast<TimestampLoggerThreadInfo*>(s_tls->get());

  if (!pti) {
    pti = new TimestampLoggerThreadInfo();
    s_tls->set(pti);
  }
  return pti;
}

} //namespace

uint32_t TimestampLoggerThreadInfo::getTimestampSize() {
  return getTimestampLoggerThreadInfo()->m_logger.getTimestampSize();
}

bool TimestampLoggerThreadInfo::getTimestampEnabled() {
  return getTimestampLoggerThreadInfo()->m_logger.getTimestampEnabled();
}

void TimestampLoggerThreadInfo::setTimestampEnabled(const bool timestampEnabled)
{
  getTimestampLoggerThreadInfo()->m_logger.setTimestampEnabled(timestampEnabled);
}

uint64_t TimestampLoggerThreadInfo::getTimestamp() {
  return getTimestampLoggerThreadInfo()->m_logger.getTimestamp();
}

void TimestampLoggerThreadInfo::logTimestamp(const void* tstamp,
                                             const size_t tstampSize)
{
  getTimestampLoggerThreadInfo()->m_logger.logTimestamp(tstamp, tstampSize);
}
