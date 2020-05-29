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
#include "android/metrics/MetricsReporter.h"

#include <assert.h>                                    // for assert
#include <stdio.h>                                     // for FILE, stdout
#include <type_traits>                                 // for swap, integral...
#include <utility>                                     // for move

#include "android/base/Optional.h"                     // for Optional
#include "android/base/async/ThreadLooper.h"           // for ThreadLooper
#include "android/base/files/PathUtils.h"              // for pj
#include "android/base/files/StdioStream.h"            // for StdioStream
#include "android/base/memory/LazyInstance.h"          // for LazyInstance
#include "android/base/threads/Async.h"                // for async
#include "android/cmdline-option.h"                    // for AndroidOptions
#include "android/metrics/AsyncMetricsReporter.h"      // for AsyncMetricsRe...
#include "android/metrics/CrashMetricsReporting.h"     // for reportCrashMet...
#include "android/metrics/FileMetricsWriter.h"         // for FileMetricsWriter
#include "android/metrics/MetricsPaths.h"              // for getSpoolDirectory
#include "android/metrics/NullMetricsReporter.h"       // for NullMetricsRep...
#include "android/metrics/PlaystoreMetricsWriter.h"    // for PlaystoreMetri...
#include "android/metrics/StudioConfig.h"              // for getAnonymizati...
#include "android/metrics/TextMetricsWriter.h"         // for TextMetricsWriter
#include "google_logs_publishing.pb.h"  // for LogEvent
#include "studio_stats.pb.h"     // for AndroidStudioE...
#include "android/utils/debug.h"                       // for dwarning
#include "android/utils/file_io.h"                     // for android_fopen
#include "picosha2.h"                                  // for hash256_one_by...

using android::base::System;

namespace android {
namespace metrics {

namespace {

static base::LazyInstance<NullMetricsReporter> sNullInstance = {};

// A small class that ensures there's always an instance of metrics reporter
// ready to process a metrics write request.
// By default it's an instance of NullMetricsReporter() which discards all
// requests, but it can be reset to anything that implements MetricsReporter
// interface.
class ReporterHolder final {
public:
    ReporterHolder() : mPtr(sNullInstance.ptr()) {}

    void reset(MetricsReporter::Ptr newPtr) {
        if (newPtr) {
            mPtr = newPtr.release();
        } else {
            // Replace the current instance with a null one and delete the old
            // reporter (if it wasn't already a null reporter.
            MetricsReporter* other = sNullInstance.ptr();
            std::swap(mPtr, other);
            if (other != sNullInstance.ptr()) {
                delete other;
            }
        }
    }

    MetricsReporter& reporter() const { return *mPtr; }

private:
    MetricsReporter* mPtr;
};

static base::LazyInstance<ReporterHolder> sInstance = {};

}  // namespace

void MetricsReporter::start(const std::string& sessionId,
                            base::StringView emulatorVersion,
                            base::StringView emulatorFullVersion,
                            base::StringView qemuVersion) {
    MetricsWriter::Ptr writer;
    if (android_cmdLineOptions->metrics_to_console) {
        writer = TextMetricsWriter::create(base::StdioStream(stdout));
    } else if (android_cmdLineOptions->metrics_collection) {
        writer = PlaystoreMetricsWriter::create(sessionId,
        base::pj(getSpoolDirectory(), "backoff_cookie.proto"));
    } else if (android_cmdLineOptions->metrics_to_file != nullptr) {
        if (FILE* out = ::android_fopen(android_cmdLineOptions->metrics_to_file,
                                        "w")) {
            writer = TextMetricsWriter::create(
                    base::StdioStream(out, base::StdioStream::kOwner));
        } else {
            dwarning("Failed to open file '%s', disabling metrics reporting",
                     android_cmdLineOptions->metrics_to_file);
        }
    } else if (studio::getUserMetricsOptIn()) {
        writer = FileMetricsWriter::create(
                getSpoolDirectory(), sessionId,
                1000,  // record limit per single file
                base::ThreadLooper::get(),
                10 * 60 * 1000);  // time limit for a single file, ms
    }

    if (!writer) {
        sInstance->reset({});
    } else {
        sInstance->reset(Ptr(new AsyncMetricsReporter(
                writer, emulatorVersion, emulatorFullVersion, qemuVersion)));

        // Run the asynchronous cleanup/reporting job now.
        base::async([] {
            const auto sessions =
                    FileMetricsWriter::finalizeAbandonedSessionFiles(
                            getSpoolDirectory());
            reportCrashMetrics(get(), sessions);
        });
    }
}

void MetricsReporter::stop(MetricsStopReason reason) {
    for (const auto& callback : sInstance->reporter().mOnExit) {
        sInstance->reporter().report(callback);
    }
    sInstance->reporter().report(
            [reason](android_studio::AndroidStudioEvent* event) {
                int crashCount = reason != METRICS_STOP_GRACEFUL ? 1 : 0;
                event->mutable_emulator_details()->set_crashes(crashCount);
            });
    sInstance->reset({});
}

void MetricsReporter::reportOnExit(Callback callback) {
    if (callback) {
        mOnExit.push_back(callback);
    }
}
MetricsReporter& MetricsReporter::get() {
    return sInstance->reporter();
}

void MetricsReporter::report(Callback callback) {
    if (!callback) {
        return;
    }
    reportConditional([callback](android_studio::AndroidStudioEvent* event) {
        callback(event);
        return true;
    });
}

MetricsReporter::MetricsReporter(bool enabled,
                                 MetricsWriter::Ptr writer,
                                 base::StringView emulatorVersion,
                                 base::StringView emulatorFullVersion,
                                 base::StringView qemuVersion)
    : mWriter(std::move(writer)),
      mEnabled(enabled),
      mStartTimeMs(System::get()->getUnixTimeUs() / 1000),
      mEmulatorVersion(emulatorVersion),
      mEmulatorFullVersion(emulatorFullVersion),
      mQemuVersion(qemuVersion) {
    assert(mWriter);
}

MetricsReporter::~MetricsReporter() = default;

bool MetricsReporter::isReportingEnabled() const {
    return mEnabled;
}

const std::string& MetricsReporter::sessionId() const {
    // Protect this from unexpected changes in the MetricsWriter interface.
    static_assert(std::is_reference<decltype(mWriter->sessionId())>::value,
                  "MetricsWriter::sessionId() must return a reference");
    return mWriter->sessionId();
}

std::string MetricsReporter::anonymize(base::StringView s) {
    picosha2::hash256_one_by_one hasher;
    hasher.process(s.begin(), s.end());
    const auto salt = this->salt();
    hasher.process(salt.begin(), salt.end());
    hasher.finish();
    return picosha2::get_hash_hex_string(hasher);
}

const base::System::Duration MetricsReporter::getStartTimeMs() {
    return mStartTimeMs;
}

void MetricsReporter::sendToWriter(android_studio::AndroidStudioEvent* event) {
    wireless_android_play_playlog::LogEvent logEvent;

    const auto timeMs = System::get()->getUnixTimeUs() / 1000;
    logEvent.set_event_time_ms(timeMs);

    if (!event->has_kind()) {
        event->set_kind(android_studio::AndroidStudioEvent::EMULATOR_PING);
    }

    event->mutable_product_details()->set_product(
            android_studio::ProductDetails::EMULATOR);
    if (!mEmulatorVersion.empty()) {
        event->mutable_product_details()->set_version(mEmulatorVersion);
    }
    if (!mEmulatorFullVersion.empty()) {
        event->mutable_product_details()->set_build(mEmulatorFullVersion);
    }
    if (!mQemuVersion.empty()) {
        event->mutable_emulator_details()->set_core_version(mQemuVersion);
    }

    const auto times = System::get()->getProcessTimes();
    event->mutable_emulator_details()->set_system_time(times.systemMs);
    event->mutable_emulator_details()->set_user_time(times.userMs);
    event->mutable_emulator_details()->set_wall_time(times.wallClockMs);

    // Only set the session ID if it isn't set: some messages might be reported
    // on behalf of a different (e.g. crashed) session.
    if (!event->has_studio_session_id()) {
        event->set_studio_session_id(sessionId());
    }
    mWriter->write(*event, &logEvent);
}

std::string MetricsReporter::salt() {
    const auto modTime =
            base::System::get()->pathModificationTime(getSettingsFilePath());
    base::AutoLock lock(mSaltLock);
    if (mSalt.empty() || modTime.valueOr(0) != mSaltFileTime) {
        auto salt = studio::getAnonymizationSalt();
        mSaltFileTime = modTime.valueOr(0);
        mSalt = std::move(salt);
    }
    return mSalt;
}

}  // namespace metrics
}  // namespace android
