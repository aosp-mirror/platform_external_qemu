
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
#include "android/emulation/control/interceptor/MetricsInterceptor.h"

#include <grpcpp/grpcpp.h>                          // for ServerRpcInfo
#include <stdint.h>                                 // for uint32_t, uint8_t
#include <zlib.h>                                   // for crc32
#include <functional>                               // for __base

#include "aemu/base/Log.h"                       // for LogStream, LOG
#include "aemu/base/memory/LazyInstance.h"       // for LazyInstance, LAZ...
#include "android/metrics/MetricsReporter.h"        // for MetricsReporter
#include "studio_stats.pb.h"  // for EmulatorGrpc, And...

namespace android {
namespace control {
namespace interceptor {

using android::base::AutoLock;
using android::metrics::MetricsReporter;
using namespace android_studio;

static void reportMetrics(const InvocationRecord& loginfo) {
    InvocationMetrics::get()->record(loginfo);
}

static base::LazyInstance<InvocationMetrics> sInvocationMetrics =
        LAZY_INSTANCE_INIT;

void InvocationMetrics::record(const InvocationRecord& invocation) {
    AutoLock lock(mLock);
    bool registerCallback = mMetrics.count(invocation.method) == 0;
    MethodMetrics& metrics = mMetrics[invocation.method];

    // To minimize data transfer we only collect send/receive for non unary
    // requests.
    if (invocation.type == ServerRpcInfo::Type::CLIENT_STREAMING ||
        invocation.type == ServerRpcInfo::Type::BIDI_STREAMING) {
        metrics.recvBytes.addSample(invocation.rcvBytes);
    }
    if (invocation.type == ServerRpcInfo::Type::SERVER_STREAMING ||
        invocation.type == ServerRpcInfo::Type::BIDI_STREAMING) {
        metrics.sndBytes.addSample(invocation.sndBytes);
    }

    metrics.duration.addSample(((double)invocation.duration) / 1000);
    if (invocation.status.error_code() == 0) {
        metrics.success++;
    } else {
        metrics.fail++;
    }

    if (registerCallback) {
        reportMetric(invocation.method, metrics);
    }
}

void InvocationMetrics::reportMetric(const std::string& name,
                                     const MethodMetrics& metric) {
    MetricsReporter::get().reportOnExit(
            [name, &metric](android_studio::AndroidStudioEvent* event) {
                uint32_t crc =
                        crc32(0, reinterpret_cast<const uint8_t*>(name.data()),
                              name.size());
                auto grpc = event->mutable_emulator_details()->mutable_grpc();
                grpc->set_call_id(crc);
                grpc->set_requests(metric.success + metric.fail);
                grpc->set_failures(metric.fail);
                metric.recvBytes.fillMetricsEvent(
                        grpc->mutable_rcv_bytes_estimate());
                metric.sndBytes.fillMetricsEvent(
                        grpc->mutable_snd_bytes_estimate());
                metric.duration.fillMetricsEvent(grpc->mutable_duration());
                LOG(DEBUG) << "Sending metric [" << name
                             << "]: " << grpc->ShortDebugString();
            });
}

InvocationMetrics* InvocationMetrics::get() {
    return sInvocationMetrics.ptr();
}

MetricsInterceptorFactory::MetricsInterceptorFactory()
    : LoggingInterceptorFactory(reportMetrics) {}
}  // namespace interceptor
}  // namespace control
}  // namespace android
