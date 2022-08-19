// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/metrics/AsyncMetricsReporter.h"

#include "android/metrics/MetricsLogging.h"
#include "studio_stats.pb.h"

#include <string_view>

namespace android {
namespace metrics {

AsyncMetricsReporter::AsyncMetricsReporter(MetricsWriter::Ptr writer,
                                           std::string_view emulatorVersion,
                                           std::string_view emulatorFullVersion,
                                           std::string_view qemuVersion)
    : MetricsReporter(true,
                      std::move(writer),
                      emulatorVersion,
                      emulatorFullVersion,
                      qemuVersion),
      mWorkerThread([this]() { worker(); }) {
    D("created");
    mWorkerThread.start();
}

AsyncMetricsReporter::~AsyncMetricsReporter() {
    // Send an empty callback as a stop request.
    D("sending a stop request to worker");
    mCallbackQueue.send({});
    mWorkerThread.wait();
}

void AsyncMetricsReporter::reportConditional(ConditionalCallback callback) {
    if (callback) {
        D("sending a callback for reporting to worker");
        mCallbackQueue.send(std::move(callback));
    }
}

void AsyncMetricsReporter::finishPendingReports() {
    // Send a 'fake' callback that reports nothing, so we can be sure that if
    // it's out of the queue then all pending callbacks were processed.
    mCallbackQueue.send(
            [](android_studio::AndroidStudioEvent*) { return false; });
    mCallbackQueue.waitForEmpty();
}

void AsyncMetricsReporter::worker() {
    while (auto cb = mCallbackQueue.receive()) {
        if (!*cb) {
            D("received a stop request");
            break;
        }
        android_studio::AndroidStudioEvent event;
        if ((*cb)(&event)) {
            sendToWriter(&event);
        }
    }
}

}  // namespace metrics
}  // namespace android
