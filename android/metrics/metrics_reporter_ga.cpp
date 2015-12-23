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

#include "android/base/String.h"
#include "android/base/StringFormat.h"
#include "android/curl-support.h"
#include "android/metrics/studio-helper.h"
#include "android/utils/compiler.h"
#include "android/utils/debug.h"

#include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>

#define mwarning(fmt, ...) \
    dwarning("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

// Useful knob for debugging: When set to true, a Google Analytics validation
// server is used to validate the post request we send, and the parsing result
// returned by the server is dumped to stdout.
static const bool use_debug_ga_server = false;

int formatGAPostData(char** ptr, const AndroidMetrics* metrics) {
    using android::base::StringFormat;

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

    char* ga_client_id = android_studio_get_installation_id();

    // TODO(pprabhu) Decide whether we want to report gpu_enabled.
    auto args = StringFormat(
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
    free(ga_client_id);

    const int result = args.size();
    *ptr = args.release();
    return result;
}

/* typedef'ed to: androidMetricsUploaderFunction */
bool androidMetrics_uploadMetricsGA(const AndroidMetrics* metrics) {
    static const char analytics_url[] =
            "https://ssl.google-analytics.com/collect";
    static const char analytics_url_debug[] =
            "https://ssl.google-analytics.com/debug/collect";

    char* fields = nullptr;
    if (formatGAPostData(&fields, metrics) < 0) {
        mwarning("Failed to allocate memory for a request");
        return false;
    }

    bool success = true;
    const char* url = use_debug_ga_server ? analytics_url_debug : analytics_url;
    char* error = nullptr;
    if (!curl_download_null(url, fields, false, &error)) {
        mwarning("Could not upload metrics: %s", error);
        ::free(error);
        success = false;
    }

    android_free(fields);
    return success;
}
