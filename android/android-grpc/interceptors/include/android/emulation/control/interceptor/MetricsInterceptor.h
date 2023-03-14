// Copyright (C) 2019 The Android Open Source Project
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
#include <string>
#include <unordered_map>

#include "aemu/base/synchronization/Lock.h"
#include "android/emulation/control/interceptor/LoggingInterceptor.h"
#include "android/metrics/Percentiles.h"

namespace android {
namespace control {
namespace interceptor {

using android::base::Lock;
using android::metrics::Percentiles;

struct MethodMetrics {
    // Note that we bucketize very quick, as we don't want to send
    // out a large amount of data.
    // Tracling on additional percentile will cost 128 bytes.

    // Tracks bytes received for streaming reads.
    Percentiles recvBytes{32, {0.9}};

    // Tracks bytes send for streaming writes.
    Percentiles sndBytes{32, {0.9}};

    // Duration of the total call length.
    Percentiles duration{32, {0.9}};

    // Number of requests for which Status::OK is true.
    int success;

    int fail;
};

// Singleton that keeps track of invocation metrics, the metrics will be
// posted upon termination of the emulator.
class InvocationMetrics {
public:
    // Record the metric.
    void record(const InvocationRecord& invocation);
    static InvocationMetrics* get();

private:
    void reportMetric(const std::string& name, const MethodMetrics& metric);
    std::unordered_map<std::string, MethodMetrics> mMetrics;
    Lock mLock;
};

// A MetricsInterceptorFactory attached to a gRPC server/client will
// collect metrics of every request and post them when the emulator
// exits.
class MetricsInterceptorFactory : public LoggingInterceptorFactory {
public:
    MetricsInterceptorFactory();
};

}  // namespace interceptor
}  // namespace control
}  // namespace android