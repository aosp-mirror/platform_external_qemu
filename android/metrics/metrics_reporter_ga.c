// Copyright 2015 The Android Open Source Project
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

#include "android/metrics/metrics_reporter_ga.h"
#include "android/metrics/internal/metrics_reporter_ga_internal.h"

#include "android/curl-support.h"
#include "android/utils/compiler.h"
#include "android/utils/debug.h"

#include <curl/curl.h>

#include <stdio.h>

#define mwarning(fmt, ...) \
    dwarning("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

// Useful knob for debugging: When set to true, a Google Analytics validation
// server is used to validate the post request we send, and the parsing result
// returned by the server is dumped to stdout.
static const bool use_debug_ga_server = false;

int formatGAPostData(char** ptr, const AndroidMetrics* metrics) {
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

    // Custom dimensions should match the index given in Analytics.
    static const char ga_cd_guest_arch_key[] = "cd6";

    // Base Google Analytics values.
    static const char ga_protocol_version[] = "1";
    static const char ga_hit_type_event[] = "event";
    // We're sharing this app with our peers.
    static const char ga_application_name[] = "Android Studio";
    // TODO(pprabhu) Send to debug Analytics project in debug builds.
    static const char ga_tracking_id[] = "UA-19996407-3";
    // TODO(pprabhu) Get anonymus UUID here, preferably the same one as studio.
    static const char ga_client_id[] = "default-client";

    // Crash reporting GA keys / values.
    static const char ga_event_category_emulator[] = "emulator";
    static const char ga_event_action_single_run_info[] = "singleRunCrashInfo";
    static const char ga_event_label_crash_detected[] = "crashDetected";
    static const char ga_event_label_clean_exit[] = "cleanExit";
    // Custom metrics should match the index given in Analytics.
    static const char ga_cm_user_time_key[] = "cm2";
    static const char ga_cm_system_time_key[] = "cm3";

    const char* label = metrics->is_dirty ? ga_event_label_crash_detected
                                          : ga_event_label_clean_exit;
    // TODO(pprabhu) Decide whether we want to report gpu_enabled.
    return asprintf(ptr,
                    "%s=%s&%s=%s&%s=%s&%s=%s&%s=%s"
                    "&%s=%s"
                    "&%s=%s&%s=%s&%s=%s&%s=%s"
                    "&%s=%" PRId64 "&%s=%" PRId64,
                    ga_protocol_version_key, ga_protocol_version,
                    ga_tracking_id_key, ga_tracking_id, ga_application_name_key,
                    ga_application_name, ga_application_version_key,
                    metrics->emulator_version, ga_client_id_key, ga_client_id,

                    ga_cd_guest_arch_key, metrics->guest_arch,

                    ga_hit_type_key, ga_hit_type_event, ga_event_category_key,
                    ga_event_category_emulator, ga_event_action_key,
                    ga_event_action_single_run_info, ga_event_label_key, label,

                    ga_cm_user_time_key, metrics->user_time,
                    ga_cm_system_time_key, metrics->system_time);
}

// Dummy write function to pass to curl to avoid dumping the returned output to
// stdout.
static size_t curlWriteFunction(CURL* handle,
                                size_t size,
                                size_t nmemb,
                                void* userdata) {
    // Report that we "took care of" all the data provided.
    return size * nmemb;
}

/* typedef'ed to: androidMetricsUploaderFunction */
bool androidMetrics_uploadMetricsGA(const AndroidMetrics* metrics) {
    static const char analytics_url[] =
            "https://ssl.google-analytics.com/collect";
    static const char analytics_url_debug[] =
            "https://ssl.google-analytics.com/debug/collect";

    bool success = true;
    CURL* const curl = curl_easy_default_init();
    CURLcode curlRes;
    if (!curl) {
        return false;
    }

    char* fields;
    if (formatGAPostData(&fields, metrics) < 0) {
        mwarning("Failed to allocate memory for a request");
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
    if (use_debug_ga_server) {
        curl_easy_setopt(curl, CURLOPT_URL, analytics_url_debug);
    } else {
        curl_easy_setopt(curl, CURLOPT_URL, analytics_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteFunction);
    }
    curlRes = curl_easy_perform(curl);
    if (curlRes != CURLE_OK) {
        success = false;
        mwarning("curl_easy_perform() failed with code %d (%s)", curlRes,
                 curl_easy_strerror(curlRes));
    }

    long http_response = 0;
    curlRes = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response);
    if (curlRes == CURLE_OK) {
        if (http_response != 200) {
            mwarning("Got HTTP response code %ld", http_response);
            success = false;
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
