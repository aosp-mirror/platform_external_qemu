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

#include "TimestampAggregatorThreadInfo.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/thread_store.h"

namespace {

class TimestampAggregatorThreadStore : public ::emugl::ThreadStore {
public:
 TimestampAggregatorThreadStore() : ::emugl::ThreadStore(&destructor) {}

private:
 static void destructor(void* value) {
   delete static_cast<TimestampAggregatorThreadInfo*>(value);
 }
};

static ::emugl::LazyInstance<TimestampAggregatorThreadStore> s_tls =
        LAZY_INSTANCE_INIT;

TimestampAggregatorThreadInfo* getTimestampAggregatorThreadInfo() {
  TimestampAggregatorThreadInfo* pti =
      static_cast<TimestampAggregatorThreadInfo*>(s_tls->get());

  if (!pti) {
    pti = new TimestampAggregatorThreadInfo();
    s_tls->set(pti);
  }
  return pti;
}

} //namespace

size_t TimestampAggregatorThreadInfo::getTimestampSize() {
  return getTimestampAggregatorThreadInfo()->logger.getTimestampSize();
}

void TimestampAggregatorThreadInfo::setTimestampEnabled(const bool timestampEnabled)
{
  getTimestampAggregatorThreadInfo()->logger.setTimestampEnabled(timestampEnabled);
}

void TimestampAggregatorThreadInfo::logTimestamp(void* ptr) {
  getTimestampAggregatorThreadInfo()->logger.logTimestamp(ptr);
}
