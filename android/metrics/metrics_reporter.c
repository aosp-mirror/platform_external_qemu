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

#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/filelock.h"
#include "android/utils/ini.h"
#include "android/utils/path.h"
#include "android/utils/string.h"
#include "android/utils/system.h"

#include "curl/curl.h"

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

    return 0 == curl_global_init(CURL_GLOBAL_ALL);
}

/* Make sure this is safe to call without ever calling _moduleInit */
void androidMetrics_moduleFini(void) {
    curl_global_cleanup();
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
    metrics_timer = android_alloc(sizeof(*metrics_timer));
    if (!metrics_timer) {
        dwarning("Failed to allocate metrics timer (OOM?).");
        return 0;
    }
    loopTimer_init(metrics_timer, metrics_looper, &on_metrics_timer,
                   metrics_timer);
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
        loopTimer_done(metrics_timer);
        AFREE(metrics_timer);
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

// /////////////////////////////////////////////////////////////////////////////
// Google Analytics reporting.

// This is a useful macro to debug changes in the google analytics HTTP post
// request. When defined, we send the request to a Google Analytics debug
// server, and print the returned parsing information / error from the server
// onto stdout. Uncomment to debug:
//
// #define GA_VALIDATE_HITS

#ifdef GA_VALIDATE_HITS
static const char analytics_url[] =
        "https://ssl.google-analytics.com/debug/collect";
#else   // !defined(GA_VALIDATE_HITS)
static const char analytics_url[] = "https://ssl.google-analytics.com/collect";
#endif  // !defined(GA_VALIDATE_HITS)

// Base GA keys.
static const char ga_protocol_version_key[] = "v";
static const char ga_tracking_id_key[] = "tid";
static const char ga_application_name_key[] = "an";
static const char ga_application_version_key[] = "av";
static const char ga_client_id_key[] = "cid";
static const char ga_hit_type_key[] = "t";
static const char ga_event_category_key[] = "ec";
static const char ga_event_action_key[] = "ea";
static const char ga_event_label_key[] = "el";
static const char ga_event_value_key[] = "ev";

// Custom dimensions should match the index given in Analytics.
static const char ga_cd_guest_arch_key[] = "cd6";
static const char ga_cd_emulator_version_key[] = "cd7";

// Base Google Analytics values.
static const char ga_protocol_version[] = "1";
static const char ga_hit_type_event[] = "event";
// We're sharing this app with our peers.
static const char ga_application_name[] = "Android Studio";
#ifdef NDEBUG
// TODO(pprabhu) Verfiy this is used in prod environment. I had trouble here.
static const char ga_tracking_id[] = "UA-19996407-3";
// TODO(pprabhu) Get anonymus UUID here, preferably the same one as studio.
static const char ga_client_id[] = "ndebug-client";
#else
static const char ga_tracking_id[] = "UA-44790371-1";
static const char ga_client_id[] = "debug-client";
#endif

// Crash reporting GA keys / values.
static const char ga_event_category_emulator[] = "emulator";
static const char ga_event_action_single_run_info[] = "singleRunInfo";
static const char ga_event_label_crash_detected[] = "crashDetected";
// Custom metrics should match the index given in Analytics.
static const char ga_cm_user_time_key[] = "cm2";
static const char ga_cm_system_time_key[] = "cm3";

static size_t formatGAPostData(char* ptr,
                               size_t n,
                               const AndroidMetrics* metrics) {
    // TODO(pprabhu) Decide whether we want to report gpu_enabled.
    return (size_t)snprintf(ptr, n,
        "%s=%s&%s=%s&%s=%s&%s=%s&%s=%s"
        "&%s=%s"
        "&%s=%s&%s=%s&%s=%s&%s=%s&%s=%d"
        "&%s=%" PRId64 "&%s=%" PRId64,
        ga_protocol_version_key, ga_protocol_version,
        ga_tracking_id_key, ga_tracking_id,
        ga_application_name_key, ga_application_name,
        ga_application_version_key, metrics->emulator_version,
        ga_client_id_key, ga_client_id,

        ga_cd_guest_arch_key, metrics->guest_arch,

        ga_hit_type_key, ga_hit_type_event,
        ga_event_category_key, ga_event_category_emulator,
        ga_event_action_key, ga_event_action_single_run_info,
        ga_event_label_key, ga_event_label_crash_detected,
        ga_event_value_key, metrics->is_dirty,

        ga_cm_user_time_key, metrics->user_time,
        ga_cm_system_time_key, metrics->system_time);
}

#ifndef GA_VALIDATE_HITS
// We pass a dummy write function to curl to avoid dumping the returned output
// to stdout.
static size_t curlWriteFunction(CURL* handle,
                                size_t size,
                                size_t nmemb,
                                void* userdata) {
    // Report that we "took care of" all the data provided.
    return size * nmemb;
}
#endif  // defined(GA_VALIDATE_HITS)

/* typedef'ed to: androidMetricsUploaderFunction */
ABool androidMetrics_uploadMetrics(const AndroidMetrics* metrics) {
    ABool success = 1;
    CURL* const curl = curl_easy_init();
    CURLcode curlRes;
    if (!curl) {
        mwarning("Failed to initialize libcurl");
        return 0;
    }

    const size_t fieldsSize = formatGAPostData(NULL, 0, metrics) + 1;
    char* const fields = (char*)android_alloc(fieldsSize * sizeof(*fields));
    if (!fields) {
        mwarning("Failed to allocate memory for a request");
        curl_easy_cleanup(curl);
        return 0;
    }

    formatGAPostData(fields, fieldsSize, metrics);

    curl_easy_setopt(curl, CURLOPT_URL, analytics_url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
#ifndef GA_VALIDATE_HITS
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteFunction);
#endif
    curlRes = curl_easy_perform(curl);
    if (curlRes != CURLE_OK) {
        success = 0;
        mwarning("curl_easy_perform() failed with code %d (%s)", curlRes,
                 curl_easy_strerror(curlRes));
    }

    long http_response = 0;
    curlRes = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response);
    if (curlRes == CURLE_OK) {
        if (http_response != 200) {
            mwarning("Got HTTP response code %ld", http_response);
            success = 0;
        }
    } else if (curlRes == CURLE_UNKNOWN_OPTION) {
        mwarning("Can not get a valid response code: not supported");
    } else {
        mwarning("Unexpected error while checking http response: %s",
                 curl_easy_strerror(curlRes));
    }

    android_free(fields);
    curl_easy_cleanup(curl);
    return success;
}
