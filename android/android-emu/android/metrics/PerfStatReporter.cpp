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

#include "android/metrics/PerfStatReporter.h"

#include "android/CommonReportedInfo.h"
#include "android/metrics/PeriodicReporter.h"

#include "android/metrics/proto/studio_stats.pb.h"
#include "android/utils/debug.h"

#define  D(...)  do {  if (VERBOSE_CHECK(memory)) dprint(__VA_ARGS__); } while (0)

static const uint32_t kMemUsageMetricsReportIntervalMs = 60 * 1000;

static bool sStopping = false;

namespace android {
namespace metrics {

using android::base::Lock;
using android::base::System;
using std::shared_ptr;

// static
PerfStatReporter::Ptr PerfStatReporter::create(
        android::base::Looper* looper,
        android::base::Looper::Duration checkIntervalMs) {
    auto inst = Ptr(new PerfStatReporter(looper, checkIntervalMs));
    return inst;
}

PerfStatReporter::PerfStatReporter(
        android::base::Looper* looper,
        android::base::Looper::Duration checkIntervalMs)
    : mLooper(looper),
      mCheckIntervalMs(checkIntervalMs),
      // We use raw pointer to |this| instead of a shared_ptr to avoid cicrular
      // ownership. mRecurrentTask promises to cancel any outstanding tasks when
      // it's destructed.
      mRecurrentTask(looper,
                     [this]() { return checkPerfStats(); },
                     mCheckIntervalMs) {

    // Also start up a periodic reporter that takes whatever is in the
    // current struct.
    using android::metrics::PeriodicReporter;
    PeriodicReporter::get().addTask(kMemUsageMetricsReportIntervalMs,
        [this](android_studio::AndroidStudioEvent* event) {

            if (sStopping) return true;

            android_studio::EmulatorPerformanceStats* perfStatProto =
                event->mutable_emulator_performance_stats();

            fillProto(perfStatProto);

            CommonReportedInfo::setPerformanceStats(perfStatProto);
            return true;
        });

    // Don't call start() here: start() launches a parallel task that calls
    // shared_from_this(), which needs at least one shared pointer owning
    // |this|. We can't guarantee that until the constructor call returns.
}

PerfStatReporter::~PerfStatReporter() {
    stop();
}

void PerfStatReporter::start() {
    mRecurrentTask.start();
}

void PerfStatReporter::stop() {
    sStopping = true;
    mRecurrentTask.stopAndWait();
}

void PerfStatReporter::fillProto(android_studio::EmulatorPerformanceStats* perfStatProto) {

    android::base::AutoLock lock(mLock);
    System::MemUsage memUsageForMetrics = mCurrUsage;
    lock.unlock();

    auto memUsageProto = perfStatProto->add_memory_usage();
    memUsageProto->set_resident_memory(memUsageForMetrics.resident);
    memUsageProto->set_resident_memory_max(memUsageForMetrics.resident_max);
    memUsageProto->set_virtual_memory(memUsageForMetrics.virt);
    memUsageProto->set_virtual_memory_max(memUsageForMetrics.virt_max);
    memUsageProto->set_total_phys_memory(memUsageForMetrics.total_phys_memory);
    memUsageProto->set_total_page_file(memUsageForMetrics.total_page_file);
}

bool PerfStatReporter::checkPerfStats() {
    System::MemUsage usage = System::get()->getMemUsage();
    uint64_t uptime = System::get()->getProcessTimes().wallClockMs;

    D("MemoryReport: uptime: %" PRIu64 ", Res/ResMax/Virt/VirtMax: "
      "%" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64,
      uptime,
      usage.resident,
      usage.resident_max,
      usage.virt,
      usage.virt_max);

    android::base::AutoLock lock(mLock);
    mCurrUsage = usage;

    return true;
}

}  // namespace metrics
}  // namespace android

