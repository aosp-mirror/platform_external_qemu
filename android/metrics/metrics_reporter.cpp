/* Copyright (C) 2015 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android/metrics/metrics_reporter.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/files/IniFile.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/crashreport/CrashReporter.h"
#include "android/metrics/AdbLivenessChecker.h"
#include "android/metrics/IniFileAutoFlusher.h"
#include "android/metrics/MetricsUploader.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/filelock.h"
#include "android/utils/ini.h"
#include "android/utils/path.h"
#include "android/utils/string.h"
#include "android/utils/system.h"

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#include <memory>
#define mwarning(fmt, ...) \
    dwarning("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

using android::base::System;
using android::metrics::MetricsUploader;

static const char metricsFileSuffix[] = ".yogibear";

// It is in genreal a bad idea to have global smart pointers, but we are careful
// to clean them up explicitly while exiting.
std::shared_ptr<MetricsUploader> sCrashMetricsUploader;
static android::metrics::AdbLivenessChecker* sAdbLivenessChecker;

/////////////////////////////////////////////////////////////
// Regular update timer.
static const int64_t metrics_timer_timeout_ms = 60 * 1000;  // A minute
static LoopTimer* metrics_timer = NULL;
/////////////////////////////////////////////////////////////

ABool androidMetrics_moduleInit(const char* avdHome) {
    // If you really call this twice, we'll reset first.
    androidMetrics_moduleFini();

    sCrashMetricsUploader = MetricsUploader::create(
            android::base::ThreadLooper::get(), avdHome, metricsFileSuffix);
    return bool(sCrashMetricsUploader);
}

/* Make sure this is safe to call without ever calling _moduleInit */
void androidMetrics_moduleFini(void) {
    // Must go before the inifile is cleaned up.
    delete sAdbLivenessChecker;
    sAdbLivenessChecker = nullptr;

    sCrashMetricsUploader.reset();
}

void androidMetrics_init(AndroidMetrics* androidMetrics) {
/* Don't use ANDROID_METRICS_STRASSIGN. We aren't guaranteed a sane or 0'ed
 * struct instance.
 */
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d) androidMetrics->n = d;
#define METRICS_STRING(n, s, d) androidMetrics->n = ASTRDUP(d);
#define METRICS_DURATION(n, s, d) androidMetrics->n = d;

#include "android/metrics/metrics_fields.h"
}

void androidMetrics_fini(AndroidMetrics* androidMetrics) {
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d)
#define METRICS_STRING(n, s, d) AFREE(androidMetrics->n);
#define METRICS_DURATION(n, s, d)

#include "android/metrics/metrics_fields.h"
}

ABool androidMetrics_write(const AndroidMetrics* androidMetrics) {
    if (!sCrashMetricsUploader) {
        return 0;
    }

    auto ini = sCrashMetricsUploader->metricsFile();
    const AndroidMetrics* am = androidMetrics;
/* Use magic macros to write all fields to the ini file. */
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d) ini->setInt(s, am->n);
#define METRICS_STRING(n, s, d) ini->setString(s, am->n ? am->n : "");
#define METRICS_DURATION(n, s, d) ini->setInt64(s, am->n);
#include "android/metrics/metrics_fields.h"
    return 1;
}

// Not static, exposed to tests ONLY.
ABool androidMetrics_tick() {
    int success;
    AndroidMetrics metrics;

    androidMetrics_init(&metrics);
    if (!androidMetrics_read(&metrics)) {
        return 0;
    }

    ++metrics.tick;

    const auto times = System::get()->getProcessTimes();
    metrics.user_time = times.userMs;
    metrics.system_time = times.systemMs;
    metrics.wallclock_time = times.wallClockMs;

    metrics.exit_started =
            android::crashreport::CrashReporter::get()->isInExitMode();

    success = androidMetrics_write(&metrics);
    androidMetrics_fini(&metrics);
    return success;
}

static void on_metrics_timer(void* ignored, LoopTimer* timer) {
    androidMetrics_tick();
    loopTimer_startRelative(timer, metrics_timer_timeout_ms);
}

ABool androidMetrics_keepAlive(Looper* metrics_looper,
                               int control_console_port) {
    if (!sCrashMetricsUploader) {
        return 0;
    }

    ABool success = 1;
    success &= androidMetrics_tick();

    // Initialize a timer for recurring metrics update
    metrics_timer = loopTimer_new(metrics_looper, &on_metrics_timer, NULL);
    loopTimer_startRelative(metrics_timer, metrics_timer_timeout_ms);

    auto emulatorName = android::base::StringFormat(
            "emulator-%d", control_console_port);
    sAdbLivenessChecker = new android::metrics::AdbLivenessChecker(
            android::base::ThreadLooper::get(),
            sCrashMetricsUploader->metricsFile(), emulatorName, 20 * 1000);
    sAdbLivenessChecker->start();

    return success;
}

ABool androidMetrics_seal() {
    if (!sCrashMetricsUploader) {
        return 1;
    }

    int success;
    AndroidMetrics metrics;

    if (metrics_timer != NULL) {
        loopTimer_stop(metrics_timer);
        loopTimer_free(metrics_timer);
        metrics_timer = NULL;
    }

    androidMetrics_init(&metrics);
    success = androidMetrics_read(&metrics);
    if (success) {
        metrics.is_dirty = 0;
        success = androidMetrics_write(&metrics);
        androidMetrics_fini(&metrics);
    }

    // Must go before the inifile is cleaned up.
    delete sAdbLivenessChecker;
    sAdbLivenessChecker = nullptr;
    sCrashMetricsUploader.reset();

    return success;
}

ABool androidMetrics_read(AndroidMetrics* androidMetrics) {
    if (!sCrashMetricsUploader) {
        return 0;
    }

    auto ini = sCrashMetricsUploader->metricsFile();
    AndroidMetrics* am = androidMetrics;
/* Use magic macros to write all fields to the ini file. */
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d) am->n = ini->getInt(s, d);
#define METRICS_STRING(n, s, d) \
    ANDROID_METRICS_STRASSIGN(am->n, ini->getString(s, d).c_str());
#define METRICS_DURATION(n, s, d) am->n = ini->getInt64(s, d);
#include "android/metrics/metrics_fields.h"
    return 1;
}

ABool androidMetrics_tryReportAll() {
    if (!sCrashMetricsUploader) {
        return 0;
    }
    sCrashMetricsUploader->uploadCompletedMetrics();
    return 1;
}

bool androidMetrics_update() {
    VERBOSE_PRINT(metrics, "metrics: requested metrics update");

    if (metrics_timer) {
        loopTimer_startRelative(metrics_timer, 0);
    }

    return true;
}
