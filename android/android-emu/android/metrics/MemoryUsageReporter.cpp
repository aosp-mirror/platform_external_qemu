// Copyright (C) 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/metrics/MemoryUsageReporter.h"

#include "android/base/system/System.h"
// #include "android/base/threads/ParallelTask.h"

// #include "android/metrics/proto/studio_stats.pb.h"
#include "android/utils/debug.h"

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

namespace android {
namespace metrics {

using android::base::System;
using std::shared_ptr;

// static
MemoryUsageReporter::Ptr MemoryUsageReporter::create(
        android::base::Looper* looper,
        android::base::Looper::Duration checkIntervalMs) {
    auto inst = Ptr(new MemoryUsageReporter(looper, checkIntervalMs));
    return inst;
}

MemoryUsageReporter::MemoryUsageReporter(
        android::base::Looper* looper,
        android::base::Looper::Duration checkIntervalMs)
    : mLooper(looper),
      mCheckIntervalMs(checkIntervalMs),
      // We use raw pointer to |this| instead of a shared_ptr to avoid cicrular
      // ownership. mRecurrentTask promises to cancel any outstanding tasks when
      // it's destructed.
      mRecurrentTask(looper,
                     [this]() { return checkMemoryUsage(); },
                     mCheckIntervalMs),
      mPrevWss(0) {
    // Don't call start() here: start() launches a parallel task that calls
    // shared_from_this(), which needs at least one shared pointer owning
    // |this|. We can't guarantee that until the constructor call returns.
}

MemoryUsageReporter::~MemoryUsageReporter() {
    stop();
}

void MemoryUsageReporter::start() {
    mRecurrentTask.start();
}

void MemoryUsageReporter::stop() {
    mRecurrentTask.stopAndWait();
}

bool MemoryUsageReporter::checkMemoryUsage() {
  size_t wss = System::get()->getPeakMemory();
  if (mPrevWss < wss) {
    time_t time = System::get()->getUnixTime();
    D("MemoryReport: Epoch: %lu, Peak: %lu", time, wss);
    mPrevWss = wss;
  }
  return true;
}

}  // namespace metrics
}  // namespace android

