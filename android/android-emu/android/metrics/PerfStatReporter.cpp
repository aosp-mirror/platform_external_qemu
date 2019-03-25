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
#include "android/base/CpuUsage.h"
#include "android/base/files/StdioStream.h"
#include "android/cmdline-option.h"
#include "android/globals.h"
#include "android/metrics/MetricsWriter.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/metrics/TextMetricsWriter.h"

#include "android/metrics/proto/clientanalytics.pb.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/utils/debug.h"
#include "android/utils/file_io.h"

#define  D(...)  do {  if (VERBOSE_CHECK(memory)) dprint(__VA_ARGS__); } while (0)

static const uint32_t kPerfStatReportIntervalMs = 10 * 1000;

static bool sStopping = false;

namespace android {
namespace metrics {

using android::base::CpuTime;
using android::base::CpuUsage;
using android::base::Lock;
using android::base::StdioStream;
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
    : mCurrPerfStats(new android_studio::EmulatorPerformanceStats),
      mLooper(looper),
      mWriter((android_cmdLineOptions->perf_stat != nullptr)
                      ? TextMetricsWriter::create(base::StdioStream(
                                android_fopen(android_cmdLineOptions->perf_stat,
                                              "w"),
                                base::StdioStream::kOwner))
                      : nullptr),
      mCheckIntervalMs(checkIntervalMs),
      // We use raw pointer to |this| instead of a shared_ptr to avoid cicrular
      // ownership. mRecurrentTask promises to cancel any outstanding tasks when
      // it's destructed.
      mRecurrentTask(
              looper,
              [this]() {
                  refreshPerfStats();
                  dump();
                  return true;
              },
              mCheckIntervalMs) {
    // Also start up a periodic reporter that takes whatever is in the
    // current struct.
    using android::metrics::PeriodicReporter;
    PeriodicReporter::get().addTask(kPerfStatReportIntervalMs,
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
    *perfStatProto = *mCurrPerfStats;
}

static void fillProtoMemUsage(android_studio::EmulatorPerformanceStats* stats_out);
static void fillProtoCpuUsage(android_studio::EmulatorPerformanceStats* stats_out);

void PerfStatReporter::dump() {
    if (mWriter != nullptr) {
        wireless_android_play_playlog::LogEvent logEvent;

        const auto timeMs = System::get()->getUnixTimeUs() / 1000;
        logEvent.set_event_time_ms(timeMs);
        android_studio::AndroidStudioEvent event;
        android_studio::EmulatorPerformanceStats* perfStatProto =
                event.mutable_emulator_performance_stats();
        fillProto(perfStatProto);
        mWriter->write(event, &logEvent);
    }
}

void PerfStatReporter::refreshPerfStats() {
    android::base::AutoLock lock(mLock);
    mCurrPerfStats->Clear();

    uint64_t uptime = System::get()->getProcessTimes().wallClockMs;
    mCurrPerfStats->set_process_uptime_us(uptime);
    uint64_t guest_uptime = 0; // TODO
    mCurrPerfStats->set_guest_uptime_us(guest_uptime);

    fillProtoMemUsage(mCurrPerfStats.get());
    fillProtoCpuUsage(mCurrPerfStats.get());
}

static void fillProtoMemUsage(android_studio::EmulatorPerformanceStats* stats_out) {
    System::MemUsage rawMemUsage = System::get()->getMemUsage();
    auto resources = stats_out->mutable_resource_usage();
    auto memUsageProto = resources->mutable_memory_usage();
    memUsageProto->set_resident_memory(rawMemUsage.resident);
    memUsageProto->set_resident_memory_max(rawMemUsage.resident_max);
    memUsageProto->set_virtual_memory(rawMemUsage.virt);
    memUsageProto->set_virtual_memory_max(rawMemUsage.virt_max);
    memUsageProto->set_total_phys_memory(rawMemUsage.total_phys_memory);
    memUsageProto->set_total_page_file(rawMemUsage.total_page_file);
    if (android_hw) {
        memUsageProto->set_total_guest_memory(android_hw->hw_ramSize * 1024 * 1024);
    }
    D("MemoryReport: uptime: %" PRIu64 ", Res/ResMax/Virt/VirtMax: "
      "%" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64,
      stats_out->process_uptime_us(),
      rawMemUsage.resident,
      rawMemUsage.resident_max,
      rawMemUsage.virt,
      rawMemUsage.virt_max);
}

static void fillProtoCpuUsage(android_studio::EmulatorPerformanceStats* stats_out) {
    CpuUsage* cpu = CpuUsage::get();
    auto resources = stats_out->mutable_resource_usage();
    auto mainLoopCpu = resources->mutable_main_loop_slice();

    cpu->forEachUsage(
        CpuUsage::UsageArea::MainLoop,
        [mainLoopCpu](const CpuTime& cpuTime) {
            mainLoopCpu->set_wall_time_us(cpuTime.wall_time_us);
            mainLoopCpu->set_user_time_us(cpuTime.user_time_us);
            mainLoopCpu->set_system_time_us(cpuTime.system_time_us);
        });

    cpu->forEachUsage(
        CpuUsage::UsageArea::Vcpu,
        [resources](const CpuTime& cpuTime) {
            auto vCpu = resources->add_vcpu_slices();
            vCpu->set_wall_time_us(cpuTime.wall_time_us);
            vCpu->set_user_time_us(cpuTime.user_time_us);
            vCpu->set_system_time_us(cpuTime.system_time_us);
        });
}

}  // namespace metrics
}  // namespace android

