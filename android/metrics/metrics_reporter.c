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

#include "android/metrics/internal/metrics_reporter_internal.h"
#include "android/metrics/metrics_reporter_ga.h"
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

#define mwarning(fmt, ...) \
    dwarning("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

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

static const char metricsRelativeDir[] = "metrics";
static const char metricsFilePrefix[] = "metrics";
static const char metricsFileSuffix[] = "yogibear";

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
    pathend = bufprint(pathend, bufend, PATH_SEP "%s.%d.%s", metricsFilePrefix,
                       getpid(), metricsFileSuffix);
    if (pathend >= bufend || path_exists(path)) {
        mwarning("Failed to get a writable, unused path for metrics. Tried: %s",
                 path);
        return NULL;
    }
    /* We ignore the returned FileLock, it will be released when the process
     * exits.
     */
    if (filelock_create(path) == 0) {
        mwarning("Failed to lock file at %s. "
                 "This indicates metric file name collision.",
                 path);
        return NULL;
    }

    /* This string will be freed by androidMetrics_seal. */
    metricsFilePath = ASTRDUP(path);
    return metricsFilePath;
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
    const char* path = androidMetrics_getMetricsFilePath();
    const AndroidMetrics* am = androidMetrics;
    IniFile* ini;

    if (path == NULL) {
        return 0;
    }

    ini = iniFile_newFromMemory("", path);
    if (ini == NULL) {
        mwarning("Failed to malloc ini file.");
        return 0;
    }

/* Use magic macros to write all fields to the ini file. */
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d) iniFile_setInteger(ini, s, am->n);
#define METRICS_STRING(n, s, d) iniFile_setValue(ini, s, am->n);
#define METRICS_DURATION(n, s, d) iniFile_setInt64(ini, s, am->n);

#include "android/metrics/metrics_fields.h"

    if (iniFile_saveToFile(ini, path) != 0) {
        mwarning("Failed to save to file at %s", path);
        iniFile_free(ini);
        return 0;
    }
    iniFile_free(ini);
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
    metrics.user_time = get_user_time_ms();
    metrics.system_time = get_system_time_ms();

    success = androidMetrics_write(&metrics);
    androidMetrics_fini(&metrics);
    return success;
}

static void on_metrics_timer(void* state) {
    LoopTimer* const timer = (LoopTimer*)state;

    androidMetrics_tick();
    loopTimer_startRelative(timer, metrics_timer_timeout_ms);
}

ABool androidMetrics_keepAlive(Looper* metrics_looper) {
    ABool success = 1;

    success &= androidMetrics_tick();

    // Initialize a timer for recurring metrics update
    metrics_timer = loopTimer_new(metrics_looper, &on_metrics_timer, NULL);
    loopTimer_startRelative(metrics_timer, metrics_timer_timeout_ms);

    return success;
}

ABool androidMetrics_seal() {
    int success;
    AndroidMetrics metrics;

    if (metricsFilePath == NULL) {
        mwarning("seal called twice / before init.");
        return 1;
    }

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

    AFREE(metricsFilePath);
    metricsFilePath = NULL;
    return success;
}

ABool androidMetrics_readPath(AndroidMetrics* androidMetrics,
                              const char* path) {
    AndroidMetrics* am = androidMetrics;
    IniFile* ini;

    ini = iniFile_newFromFile(path);
    if (ini == NULL) {
        mwarning("Could not open metrics file %s for reading", path);
        return 0;
    }

/* Use magic macros to read all fields from the ini file.
 * Set to default for the missing fields.
 */
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d) am->n = iniFile_getInteger(ini, s, d);
#define METRICS_STRING(n, s, d)                                         \
    if (iniFile_getValue(ini, s)) {                                     \
        ANDROID_METRICS_STRASSIGN(am->n, iniFile_getString(ini, s, d)); \
    }
#define METRICS_DURATION(n, s, d) am->n = iniFile_getInt64(ini, s, d);

#include "android/metrics/metrics_fields.h"

    iniFile_free(ini);
    return 1;
}

ABool androidMetrics_read(AndroidMetrics* androidMetrics) {
    const char* path = androidMetrics_getMetricsFilePath();
    if (path == NULL) {
        return 0;
    }
    return androidMetrics_readPath(androidMetrics, path);
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
                androidMetrics_fini(&metrics);
            }
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
            androidMetrics_fini(&metrics);
        }
    }
    VERBOSE_PRINT(init, "metrics: Processed %d reports.", num_reports);
    VERBOSE_PRINT(init, "metrics: Uploaded %d reports successfully.",
                  num_uploads);

    return success;
}

void androidMetrics_injectUploader(
        androidMetricsUploaderFunction uploaderFunction) {
    testUploader = uploaderFunction;
}

/* typedef'ed to: androidMetricsUploaderFunction */
ABool androidMetrics_uploadMetrics(const AndroidMetrics* metrics) {
    return (ABool)androidMetrics_uploadMetricsGA(metrics);
}
