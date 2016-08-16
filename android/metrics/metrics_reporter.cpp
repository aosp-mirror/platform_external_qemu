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
#include "android/metrics/metrics_reporter_callbacks.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/files/IniFile.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/crashreport/CrashReporter.h"
#include "android/metrics/AdbLivenessChecker.h"
#include "android/metrics/internal/metrics_reporter_internal.h"
#include "android/metrics/IniFileAutoFlusher.h"
#include "android/metrics/metrics_reporter_toolbar.h"
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
#include <string>

#include <assert.h>

#define mwarning(fmt, ...) \
    dwarning("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

using android::base::System;

/* The number of metrics files to batch together when uploading metrics.
 * Tune this so that we get enough reports as well as each report consists of a
 * fair number of emulator runs.
 */
#define METRICS_REPORTING_THRESHOLD 3

/* The global variable containing the location to the metrics file in use in the
 * current emulator instance
 */
static char* metricsDirPath;
static char* metricsFilePath;
// It's generally bad to have global smart pointers. But we are careful to
// reset them explicitly when exiting.
static android::metrics::IniFileAutoFlusher::Ptr sMetricsFileFlusher;
static std::shared_ptr<android::base::IniFile> sMetricsIniFile;
static std::shared_ptr<android::metrics::AdbLivenessChecker> sAdbLivenessChecker;

static const char metricsRelativeDir[] = "metrics";
static const char metricsFilePrefix[] = "metrics";
static const char metricsFileSuffix[] = "yogibear";

// I will burn in hell for this global.
// TODO(pprabhu) Replace by a sane object.
std::vector<android::metrics::TickCallback> sTickCallbacks;

/////////////////////////////////////////////////////////////
// Regular update timer.
static const int64_t metrics_timer_timeout_ms = 60 * 1000;  // A minute
static LoopTimer* metrics_timer = NULL;
/////////////////////////////////////////////////////////////

/* Global injections for testing purposes. */
static androidMetricsUploaderFunction testUploader = NULL;

ABool androidMetrics_moduleInit(const char* avdHome) {
    char path[PATH_MAX], *pathend = path, *bufend = pathend + sizeof(path);

    pathend = bufprint(pathend, bufend, "%s", avdHome);
    if (pathend >= bufend || !path_is_dir(path)) {
        mwarning("Failed to get a valid avd home directory.");
        return 0;
    }

    pathend = bufprint(pathend, bufend, PATH_SEP "%s", metricsRelativeDir);
    if (pathend >= bufend || path_mkdir_if_needed(path, 0744) != 0) {
        mwarning("Failed to create a valid metrics home directory.");
        return 0;
    }

    metricsDirPath = ASTRDUP(path);
    return 1;
}

/* Make sure this is safe to call without ever calling _moduleInit */
void androidMetrics_moduleFini(void) {
    sAdbLivenessChecker.reset();
    sMetricsIniFile.reset();
    sMetricsFileFlusher.reset();
    AFREE(metricsDirPath);
    AFREE(metricsFilePath);
    metricsDirPath = metricsFilePath = NULL;
}

/* Generate the path to the metrics file to use in the emulator instance.
 * - Ensures that the path is sane, unused so far.
 * - Locks the file path so that no other emulator instance can claim it. This
 *   lock should be held as long as we intend to update the metrics file.
 * - Stashes away the path to a static global var, so that subsequent calls are
 *   guaranteed to return the same value.
 *
 *   Returns NULL on failure to initialize the path.
 */
const char* androidMetrics_getMetricsFilePath() {
    char path[PATH_MAX], *pathend = path, *bufend = pathend + sizeof(path);

    if (metricsFilePath != NULL) {
        return metricsFilePath;
    }

    pathend = bufprint(pathend, bufend, "%s", metricsDirPath);
    if (pathend >= bufend) {
        return NULL;
    }

    /* TODO(pprabhu) Deal with pid collisions. */
    pathend = bufprint(pathend,
                       bufend,
                       PATH_SEP "%s.%d.%s",
                       metricsFilePrefix,
                       getpid(),
                       metricsFileSuffix);
    if (pathend >= bufend || path_exists(path)) {
        mwarning("Failed to get a writable, unused path for metrics. Tried: %s",
                 path);
        return NULL;
    }
    /* We ignore the returned FileLock, it will be released when the process
     * exits.
     */
    if (filelock_create(path) == 0) {
        mwarning(
                "Failed to lock file at %s. "
                "This indicates metric file name collision.",
                path);
        return NULL;
    }

    /* The AutoFileFlusher will be stoped and path string will be freed by
     * androidMetrics_seal. */
    metricsFilePath = ASTRDUP(path);
    sMetricsFileFlusher = android::metrics::IniFileAutoFlusher::create(
            android::base::ThreadLooper::get());
    sMetricsIniFile = std::make_shared<android::base::IniFile>(metricsFilePath);
    sMetricsFileFlusher->start(sMetricsIniFile);
    return metricsFilePath;
}

void androidMetrics_init(AndroidMetrics* androidMetrics) {
/* Don't use ANDROID_METRICS_STRASSIGN. We aren't guaranteed a sane or 0'ed
 * struct instance.
 */
#undef METRICS_INT
#undef METRICS_INT64
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d) androidMetrics->n = d;
#define METRICS_INT64(n, s, d) androidMetrics->n = d;
#define METRICS_STRING(n, s, d) androidMetrics->n = ASTRDUP(d);
#define METRICS_DURATION(n, s, d) androidMetrics->n = d;

#include "android/metrics/metrics_fields.h"
}

void androidMetrics_fini(AndroidMetrics* androidMetrics) {
#undef METRICS_INT
#undef METRICS_INT64
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d)
#define METRICS_INT64(n, s, d)
#define METRICS_STRING(n, s, d) AFREE(androidMetrics->n);
#define METRICS_DURATION(n, s, d)

#include "android/metrics/metrics_fields.h"
}

ABool androidMetrics_write(const AndroidMetrics* androidMetrics) {
    if (!androidMetrics_getMetricsFilePath()) {
        return 0;
    }

    auto ini = sMetricsIniFile;
    const AndroidMetrics* am = androidMetrics;
/* Use magic macros to write all fields to the ini file. */
#undef METRICS_INT
#undef METRICS_INT64
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d) ini->setInt(s, am->n);
#define METRICS_INT64(n, s, d) ini->setInt64(s, am->n);
#define METRICS_STRING(n, s, d) ini->setString(s, am->n ? am->n : "");
#define METRICS_DURATION(n, s, d) ini->setInt64(s, am->n);
#include "android/metrics/metrics_fields.h"
    return 1;
}

// Not static, exposed to tests ONLY.
ABool androidMetrics_tick() {
    bool success = false;
    AndroidMetrics metrics;

    androidMetrics_init(&metrics);
    if (androidMetrics_read(&metrics)) {
        ++metrics.tick;

        const auto times = System::get()->getProcessTimes();
        metrics.user_time = times.userMs;
        metrics.system_time = times.systemMs;
        metrics.wallclock_time = times.wallClockMs;

        // HACK: Call all registered callbacks. This is a hacky way to extend
        // regular metrics drops for now.
        // Should be replaced by a proper object that registers callbacks.
        for (const auto callback : sTickCallbacks) {
            callback(&metrics);
        }

        const auto exit_started =
                android::crashreport::CrashReporter::get()->isInExitMode();
        metrics.exit_started = exit_started ? 1 : 0;

        success = androidMetrics_write(&metrics);
    }
    androidMetrics_fini(&metrics);
    return success;
}

static void on_metrics_timer(void* ignored, LoopTimer* timer) {
    androidMetrics_tick();
    loopTimer_startRelative(timer, metrics_timer_timeout_ms);
}

ABool androidMetrics_keepAlive(Looper* metrics_looper,
                               int control_console_port) {
    if (!androidMetrics_getMetricsFilePath()) {
        return false;
    }

    ABool success = 1;

    success &= androidMetrics_tick();

    // Initialize a timer for recurring metrics update
    metrics_timer = loopTimer_new(metrics_looper, &on_metrics_timer, NULL);
    loopTimer_startRelative(metrics_timer, metrics_timer_timeout_ms);

    // Make sure we've got the metrics file before creating an object the sole
    // purpose of which writing some metrics there
    assert(sMetricsIniFile);

    auto emulatorName = android::base::StringFormat(
            "emulator-%d", control_console_port);
    sAdbLivenessChecker = android::metrics::AdbLivenessChecker::create(
            android::base::ThreadLooper::get(),
            sMetricsIniFile,
            emulatorName,
            20 * 1000);
    sAdbLivenessChecker->start();

    return success;
}

ABool androidMetrics_seal() {
    int success;
    AndroidMetrics metrics;

    if (metricsFilePath == NULL) {
        return 1;
    }

    if (metrics_timer != NULL) {
        loopTimer_stop(metrics_timer);
        loopTimer_free(metrics_timer);
        metrics_timer = NULL;

        // Force a last tick so we get the most up-to-date data.
        androidMetrics_tick();
    }

    androidMetrics_init(&metrics);
    success = androidMetrics_read(&metrics);
    if (success) {
        metrics.is_dirty = 0;
        success = androidMetrics_write(&metrics);
    }
    androidMetrics_fini(&metrics);

    sAdbLivenessChecker.reset();
    sMetricsIniFile.reset();
    sMetricsFileFlusher.reset();
    AFREE(metricsFilePath);
    metricsFilePath = NULL;
    return success;
}

ABool androidMetrics_readPath(AndroidMetrics* androidMetrics,
                              const char* path) {
    AndroidMetrics* am = androidMetrics;
    CIniFile* ini;

    ini = iniFile_newFromFile(path);
    if (ini == NULL) {
        mwarning("Could not open metrics file %s for reading", path);
        return 0;
    }

/* Use magic macros to read all fields from the ini file.
 * Set to default for the missing fields.
 */
#undef METRICS_INT
#undef METRICS_INT64
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d) am->n = iniFile_getInteger(ini, s, d);
#define METRICS_INT64(n, s, d) am->n = iniFile_getInt64(ini, s, d);
#define METRICS_STRING(n, s, d) \
    if (iniFile_hasKey(ini, s)) { \
        ANDROID_METRICS_STRASSIGN_MALLOCED(am->n, iniFile_getString(ini, s, d)); \
    }
#define METRICS_DURATION(n, s, d) am->n = iniFile_getInt64(ini, s, d);

#include "android/metrics/metrics_fields.h"

    iniFile_free(ini);
    return 1;
}

ABool androidMetrics_read(AndroidMetrics* androidMetrics) {
    if (!androidMetrics_getMetricsFilePath()) {
        return 0;
    }

    auto ini = sMetricsIniFile;
    AndroidMetrics* am = androidMetrics;
/* Use magic macros to write all fields to the ini file. */
#undef METRICS_INT
#undef METRICS_INT64
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d) am->n = ini->getInt(s, d);
#define METRICS_INT64(n, s, d) am->n = ini->getInt64(s, d);
#define METRICS_STRING(n, s, d) \
    ANDROID_METRICS_STRASSIGN(am->n, ini->getString(s, d).c_str());
#define METRICS_DURATION(n, s, d) am->n = ini->getInt64(s, d);
#include "android/metrics/metrics_fields.h"
    return 1;
}

/* Forward declaration. */
ABool androidMetrics_uploadMetrics(const AndroidMetrics* metrics);

ABool androidMetrics_tryReportAll() {
    DirScanner* avd_dir = NULL;
    const char* file_path;
    char* duped_file_path;
    char* my_absolute_path;
    FileLock* file_lock;
    ABool success = 1, upload_success;
    int num_reports = 0, num_uploads = 0;
    AndroidMetrics metrics;

    avd_dir = dirScanner_new(metricsDirPath);
    my_absolute_path = path_get_absolute(androidMetrics_getMetricsFilePath());
    if (avd_dir == NULL || my_absolute_path == NULL) {
        mwarning("Failed to allocate objects. OOM?");
        dirScanner_free(avd_dir);
        AFREE(my_absolute_path);
        return 0;
    }

    /* Inject uploader function for testing purposes. */
    const androidMetricsUploaderFunction uploader_function =
            (testUploader != NULL) ? testUploader
                                   : &androidMetrics_uploadMetrics;

    while ((file_path = dirScanner_nextFull(avd_dir)) != NULL) {
        if (!str_ends_with(file_path, metricsFileSuffix) ||
            0 == strcmp(my_absolute_path, file_path)) {
            continue;
        }

        /* Quickly ASTRDUP the returned string to workaround a bug in
         * dirScanner. As the contents of the scanned directory change, the
         * pointer returned by dirScanner gets changed behind our back.
         * Even taking a filelock changes the directory contents...
         */
        duped_file_path = ASTRDUP(file_path);

        file_lock = filelock_create(duped_file_path);
        /* We may not get this file_lock if the emulator process that created
         * the metrics file is still running. This is by design -- we don't want
         * to process partial metrics files.
         */
        if (file_lock != NULL) {
            ++num_reports;
            upload_success = 0;
            androidMetrics_init(&metrics);
            if (androidMetrics_readPath(&metrics, duped_file_path)) {
                /* Make sure that the reporting process actually did some
                 * reporting. */
                if (metrics.tick > 0) {
                    upload_success = uploader_function(&metrics);
                }
            }
            androidMetrics_fini(&metrics);
            success &= upload_success;
            if (upload_success) {
                ++num_uploads;
            }

            /* Current strategy is to delete the metrics dump on failed upload,
             * noting that we missed the metric. This protects us from leaving
             * behind too many files if we consistently fail to upload.
             */
            success &= (0 == path_delete_file(duped_file_path));
            filelock_release(file_lock);
        }
        AFREE(duped_file_path);
    }
    dirScanner_free(avd_dir);
    AFREE(my_absolute_path);

    if (num_uploads != num_reports) {
        androidMetrics_init(&metrics);
        if (androidMetrics_read(&metrics)) {
            metrics.num_failed_reports = num_reports - num_uploads;
            androidMetrics_write(&metrics);
        }
        androidMetrics_fini(&metrics);
    }
    VERBOSE_PRINT(init, "metrics: Processed %d reports.", num_reports);
    VERBOSE_PRINT(
            init, "metrics: Uploaded %d reports successfully.", num_uploads);

    return success;
}

void androidMetrics_injectUploader(
        androidMetricsUploaderFunction uploaderFunction) {
    testUploader = uploaderFunction;
}

/* typedef'ed to: androidMetricsUploaderFunction */
ABool androidMetrics_uploadMetrics(const AndroidMetrics* metrics) {
    VERBOSE_PRINT(metrics,
                  "metrics: Uploading a report with status '%s', "
                  "num failures '%d' "
                  "(version '%s/%s', sys/user/wall times '%ld/%ld/%ld').",
                  metrics->is_dirty ? "crash" : "clean",
                  metrics->num_failed_reports,
                  metrics->emulator_version,
                  metrics->core_version,
                  metrics->system_time,
                  metrics->user_time,
                  metrics->wallclock_time);

    return androidMetrics_uploadMetricsToolbar(metrics);
}

bool androidMetrics_update() {
    VERBOSE_PRINT(metrics, "metrics: requested metrics update");

    if (metrics_timer) {
        loopTimer_startRelative(metrics_timer, 0);
    }

    return true;
}

#define METRICS_ASSIGN_GPU_PROPS(metrics, i, plist)              \
    do {                                                         \
        ANDROID_METRICS_STRASSIGN(metrics->gpu##i##_make,        \
                                  plist->props[i].make);         \
        ANDROID_METRICS_STRASSIGN(metrics->gpu##i##_model,       \
                                  plist->props[i].model);        \
        ANDROID_METRICS_STRASSIGN(metrics->gpu##i##_device_id,   \
                                  plist->props[i].device_id);    \
        ANDROID_METRICS_STRASSIGN(metrics->gpu##i##_revision_id, \
                                  plist->props[i].revision_id);  \
        ANDROID_METRICS_STRASSIGN(metrics->gpu##i##_version,     \
                                  plist->props[i].version);      \
        ANDROID_METRICS_STRASSIGN(metrics->gpu##i##_renderer,    \
                                  plist->props[i].renderer);     \
    } while (0);

#define METRICS_GPUREPORT(metrics, i, plist)            \
    do {                                                \
        if (i < plist->num_gpus) {                      \
            METRICS_ASSIGN_GPU_PROPS(metrics, i, plist) \
        }                                               \
    } while (0);

void androidMetrics_populateGpuProps(AndroidMetrics* metrics,
                                     emugl_host_gpu_prop_list* plist) {
    METRICS_GPUREPORT(metrics, 0, plist);
    METRICS_GPUREPORT(metrics, 1, plist);
    METRICS_GPUREPORT(metrics, 2, plist);
    METRICS_GPUREPORT(metrics, 3, plist);
}

namespace android {
namespace metrics {

void addTickCallback(TickCallback callback) {
    sTickCallbacks.push_back(std::move(callback));
}

}  // namespace metrics
}  // namespace android
