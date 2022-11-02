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

#pragma once

#include <functional>  // for function
#include <memory>      // for unique_ptr
#include <string>      // for string
#include <string_view>
#include <vector>      // for vector

#include "aemu/base/Compiler.h"              // for DISALLOW_COPY_ASSIGN_...

#include "aemu/base/synchronization/Lock.h"  // for Lock
#include "android/base/system/System.h"         // for System, System::Duration
#include "android/metrics/MetricsWriter.h"      // for MetricsWriter, Metric...
#include "android/metrics/export.h"
#include "android/metrics/metrics.h"  // for MetricsStopReason

namespace android_studio {
class AndroidStudioEvent;
}  // namespace android_studio

namespace android {
namespace metrics {

class Reporter {
public:
    virtual ~Reporter() = default;

    // Start the Reporter
    virtual void start() = 0;
    // Stop the currently running reporter instance. This finalizes and
    // deletes all currently running tasks and reverts back to a mode where the
    // reporter collects all tasks but never runs them.
    virtual void stop() = 0;
};

//
// MetricsReporter - the main interface for metrics reporing in the emulator.
//
// To report a metric, call the report() method with a callback that will fill
// the passed |event| object with the metric values. E.g.
//
//  Screen screen = ...;
//  screen.DrawAFrame();
//  int frameTime = screen.GetFrameDrawingTime();
//  MetricsReporter::get().report([frameTime](AndroidStudioEvent* event) {
//      event->mutable_emulator_details()->set_frame_time(frameTime);
//  });
//
// This callback might be invoked either synchronously or asynchronously on a
// different thread, so it should capture all pieces of data it might need.
// It might even be never called at all - if metrics reporting is disabled -
// so make sure your code doesn't rely on that in any way.
//
// If you are aggregating metrics that you wish to report you can register a
// callback that will be invoked upon closing the emulator.
//
//  int counter = ....
//  MetricsReporter::get().reportOnExit(
//            [&counter](android_studio::AndroidStudioEvent* event) {
//                event->mutable_emulator_details()->...
//  }
//  counter++;
//
// There is one advanced method, reportConditional(): it expects a different
// type of callback, one that returns |true| if it logged anything or |false| if
// it didn't and the metric message should be discarded. This is useful when
// metric reporting requires some long-running operation, and one doesn't want
// to run it on the same thread or to run it at all if metrics are disabled,
// but based on the outcome one might decide not to report it at all.
//
// Anyway, if even reportConditional() is not for you, you can call
// isReportingEnabled() to see if it makes sense at all to report the metrics.
//

class AEMU_METRICS_API MetricsReporter {
    DISALLOW_COPY_ASSIGN_AND_MOVE(MetricsReporter);

public:
    using Ptr = std::unique_ptr<MetricsReporter>;
    using Callback = std::function<void(android_studio::AndroidStudioEvent*)>;
    using ConditionalCallback =
            std::function<bool(android_studio::AndroidStudioEvent*)>;

    static void start(const std::string& sessionId,
                      std::string_view emulatorVersion,
                      std::string_view emulatorFullVersion,
                      std::string_view qemuVersion);
    static void stop(MetricsStopReason reason);
    static MetricsReporter& get();

    virtual ~MetricsReporter();
    virtual void reportConditional(ConditionalCallback callback) = 0;
    // Wait for all pending reports to be finished.
    virtual void finishPendingReports() = 0;

    void report(Callback callback);
    // Report the metric before exiting
    void reportOnExit(Callback callback);
    // Checks if the metrics reporting is enabled for the current reporter
    // instance.
    bool isReportingEnabled() const;
    // Returns a unique identifier of the current emulator run (or session in
    // Android Studio terms).
    const std::string& sessionId() const;
    // Returns an anonymized copy of |s| string, using the Android Studio's
    // salt value + some hashing algorithm.
    std::string anonymize(std::string_view s);
    // Returns the start time.
    base::System::Duration getStartTimeMs();

protected:
    MetricsReporter(bool enabled,
                    MetricsWriter::Ptr writer,
                    std::string_view emulatorVersion,
                    std::string_view emulatorFullVersion,
                    std::string_view qemuVersion);
    void sendToWriter(android_studio::AndroidStudioEvent* event);

private:
    std::string salt();

    const MetricsWriter::Ptr mWriter;
    const bool mEnabled = false;
    const base::System::Duration mStartTimeMs;
    const std::string mEmulatorVersion;
    const std::string mEmulatorFullVersion;
    const std::string mQemuVersion;

    base::Lock mSaltLock;
    base::System::Duration mSaltFileTime = 0;
    std::string mSalt;
    std::vector<Callback> mOnExit;
};

}  // namespace metrics
}  // namespace android
