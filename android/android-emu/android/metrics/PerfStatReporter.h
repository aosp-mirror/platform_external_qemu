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

#pragma once

#include "android/base/async/Looper.h"
#include "android/base/async/RecurrentTask.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/PeriodicReporter.h"

#include <memory>

namespace android_studio {

class EmulatorPerformanceStats;

} // namespace android_studio

namespace android {
namespace metrics {

// A PerfStatReporter is an object that runs in the background to track
// usage of various resources such as RAM and CPU usage.
class PerfStatReporter final
        : public std::enable_shared_from_this<PerfStatReporter> {
public:
    using Ptr = std::shared_ptr<PerfStatReporter>;

    // Entry point to create a PerfStatReporter
    // Objects of this type are managed via shared_ptr.
    static Ptr create(
            android::base::Looper* looper,
            android::base::Looper::Duration checkIntervalMs);

    void start();
    void stop();

    ~PerfStatReporter();

protected:
    // Use |create| to correctly initialize the shared_ptr count.
    PerfStatReporter(android::base::Looper* looper,
                     android::base::Looper::Duration checkIntervalMs);

private:
    void dump();
    void refreshPerfStats();
    void fillProto(android_studio::EmulatorPerformanceStats* perfStatProto);

    std::unique_ptr<android_studio::EmulatorPerformanceStats> mCurrPerfStats;
    const MetricsWriter::Ptr mWriter;
    android::base::Looper* const mLooper;
    const android::base::Looper::Duration mCheckIntervalMs;
    android::base::RecurrentTask mRecurrentTask;
    android::metrics::PeriodicReporter::TaskToken mMetricsReportingToken;
    // TODO: Add more perf stats here.
    android::base::Lock mLock;

    DISALLOW_COPY_AND_ASSIGN(PerfStatReporter);
};

}  // namespace base
}  // namespace android

