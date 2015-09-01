/* Copyright (C) 20015 The Android Open Source Project
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

#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/filelock.h"
#include "android/utils/ini.h"
#include "android/utils/path.h"
#include "android/utils/string.h"
#include "android/utils/system.h"
#ifdef _WIN32
# include <process.h>
#else
# include <unistd.h>
#endif

#include "curl/curl.h"

# define mwarning(fmt, ...) \
        dwarning("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)


// TODO: merge in the real code

/* These macros are used to define the fields of AndroidMetrics */
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_INT64
#define METRICS_INT(n, s, d)        int n;
#define METRICS_STRING(n, s, d)     char* n;
#define METRICS_INT64(n, s, d)      int64_t n;

typedef struct {

    METRICS_STRING(
            emulator_version,
            "emulator_version",
            "unknown"
    )

    METRICS_STRING(
            guest_arch,
            "guest_arch",
            "unknown"
    )

    METRICS_INT(
            guest_gpu_enabled,
            "guest_gpu_enabled",
            -99
    )

    METRICS_INT(
            tick,
            "tick",
            0
    )

    METRICS_INT64(
            system_time,
            "system_time",
            0
    )

    METRICS_INT64(
            user_time,
            "user_time",
            0
    )

    METRICS_INT(
            is_dirty,
            "is_dirty",
            1
    )

    METRICS_INT(
            num_failed_reports,
            "num_failed_reports",
            0
    )
} AndroidMetrics;


// these two are taken from the Android Studio -
//  analytics/src/com/android/stats/AnalyticsUploader.java
static const char analytics_url[] = "https://ssl.google-analytics.com/collect";
#ifdef NDEBUG
static const char analytics_id[] = "UA-19996407-3";
#else
static const char analytics_id[] = "UA-44790371-1";
#endif

// these are specific to the emulator
static const char analytics_app[] = "Android Emulator";

static size_t formatPostFields(char* ptr, size_t n, const AndroidMetrics* metrics) {
    // format the metrics into post data string, having the following dimensions
    // and values:
    // - custom dimension 1 - guest arch
    // - custom dimension 2 - is gpu enabled
    // - event value - number of crashes
    // - custom metric 1 - user time
    // - custom metric 2 - system time

    return (size_t)snprintf(ptr, n,
        "v=%d&tid=%s&an=%s&av=%s&t=%s&ec=%s&ea=%s&cd1=%s&cd2=%s&ev=%d&cm1=%" PRId64 "&cm2=%" PRId64,
        1, analytics_id, analytics_app, metrics->emulator_version, "event", "crashTracking", "Hit",
        metrics->guest_arch, (metrics->guest_gpu_enabled ? "enabled" : "disabled"),
        metrics->num_failed_reports, metrics->user_time, metrics->system_time);
}

ABool
androidMetrics_uploadMetrics(const AndroidMetrics* metrics)
{
    CURL* const curl = curl_easy_init();
    if (!curl) {
        mwarning("Failed to initialize libcurl");
        return 0;
    }

    ABool res = 0;
    const size_t fieldsSize = formatPostFields(NULL, 0, metrics);
    char* const fields = (char*)android_alloc(fieldsSize * sizeof(*fields));
    if (!fields) {
        mwarning("Failed to allocate memory for a request");
    } else {
        formatPostFields(fields, fieldsSize, metrics);

        curl_easy_setopt(curl, CURLOPT_URL, analytics_url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);

        const CURLcode curlRes = curl_easy_perform(curl);
        if (curlRes != CURLE_OK) {
            mwarning("curl_easy_perform() failed with code %d (%s)",
                curlRes, curl_easy_strerror(curlRes));
        } else {
            res = 1;
        }
        android_free(fields);
    }

    curl_easy_cleanup(curl);
    return res;
}