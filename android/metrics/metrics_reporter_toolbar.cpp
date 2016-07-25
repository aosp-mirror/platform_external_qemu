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

#include "android/metrics/metrics_reporter_toolbar.h"
#include "android/metrics/internal/metrics_reporter_toolbar_internal.h"

#include "android/base/misc/StringUtils.h"
#include "android/base/Uri.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/curl-support.h"
#include "android/metrics/StudioConfig.h"
#include "android/metrics/studio-config.h"
#include "android/utils/compiler.h"
#include "android/utils/debug.h"
#include "android/utils/uri.h"

#include <string>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define mwarning(fmt, ...) \
    dwarning("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

using android::base::ScopedCPtr;
using android::base::Uri;
using android::studio::UpdateChannel;

static const int kUrlLengthLimit = 8000;
static void url_limit_append(std::string* dst, const std::string& src) {
    if (dst->size() + src.size() > kUrlLengthLimit) {
        return;
    }
    *dst += src;
}

template <class Val>
static void urlAddKeyVal(std::string* dst,
                         const char* format,
                         const char* key,
                         const Val& val) {
    url_limit_append(dst, Uri::FormatEncodeArguments(format, key, val));
}

static void url_addstrval(std::string* dst, const char* key, const char* val) {
    urlAddKeyVal(dst, "&%s=%s", key, val);
}

static void url_addintval(std::string* dst, const char* key, int val) {
    urlAddKeyVal(dst, "&%s=%d", key, val);
}

static void url_addi64val(std::string* dst, const char* key, uint64_t val) {
    urlAddKeyVal(dst, "&%s=%" PRId64, key, val);
}

#define URL_GPU_FORMAT(dst, gpu)                                             \
    do {                                                                     \
        url_addstrval(dst, #gpu "_make", metrics->gpu##_make);               \
        url_addstrval(dst, #gpu "_model", metrics->gpu##_model);             \
        url_addstrval(dst, #gpu "_device_id", metrics->gpu##_device_id);     \
        url_addstrval(dst, #gpu "_revision_id", metrics->gpu##_revision_id); \
        url_addstrval(dst, #gpu "_version", metrics->gpu##_version);         \
        url_addstrval(dst, #gpu "_renderer", metrics->gpu##_renderer);       \
    } while (0);

// Keep in sync with backend enum in .../processed_logs.proto
int toUpdateChannelToolbarEnum(UpdateChannel channel) {
    switch (channel) {
        case UpdateChannel::Stable:
            return 1;
        case UpdateChannel::Beta:
            return 2;
        case UpdateChannel::Dev:
            return 3;
        case UpdateChannel::Canary:
            return 4;
        default:
            return 0;
    }
}

int formatToolbarGetUrl(char** ptr,
                        const char* url,
                        const AndroidMetrics* metrics) {

    // This is of the form: androidsdk_<product_name>_<event_name>
    static const char product_name[] = "androidsdk_emu_crash";
    static const char guest_arch_key[] = "guest_arch";
    static const char guest_api_level_key[] = "guest_api_level";
    static const char system_time_key[] = "system_time";
    static const char user_time_key[] = "user_time";
    static const char adb_liveness_key[] = "adb_liveness";
    // These keys are the same as AndroidStudio already uses.
    static const char client_id_key[] = "id";
    static const char version_key[] = "version";
    static const char core_version_key[] = "core_version";
    static const char num_crashes_key[] = "exf";
    static const char opengl_alive_key[] = "opengl_alive";
    // Matches the key used by the update ping.
    static const char host_os_key[] = "os";

    static const char update_channel_key[] = "update_channel";
    static const char wallclock_time_key[] = "wall_time";
    static const char user_actions_key[] = "user_actions";
    static const char exit_started_key[] = "exit_started";

    // Selected renderer backend
    static const char renderer_key[] = "renderer";

    // Guest GPU information
    static const char guest_gl_vendor_key[] = "ggl_vendor";
    static const char guest_gl_renderer_key[] = "ggl_renderer";
    static const char guest_gl_version_key[] = "ggl_version";

    assert(ptr != NULL);
    assert(*ptr == NULL);

    std::string fullUrl = url;

    android::base::ScopedCPtr<char> client_id(
            android_studio_get_installation_id());

    fullUrl += Uri::FormatEncodeArguments("?as=%s", product_name);
    // Keep the URL length under kUrlLengthLimit.
    url_addstrval(&fullUrl, version_key, metrics->emulator_version);
    url_addstrval(&fullUrl, core_version_key, metrics->core_version);
    url_addi64val(&fullUrl, update_channel_key,
                    toUpdateChannelToolbarEnum(android::studio::updateChannel()));
    url_addstrval(&fullUrl, host_os_key, metrics->host_os_type);
    url_addstrval(&fullUrl, client_id_key, client_id.get());
    url_addstrval(&fullUrl, guest_arch_key, metrics->guest_arch);
    url_addintval(&fullUrl, guest_api_level_key, metrics->guest_api_level);
    url_addintval(&fullUrl, num_crashes_key, metrics->is_dirty ? 1 : 0);
    url_addintval(&fullUrl, opengl_alive_key, metrics->opengl_alive);
    url_addi64val(&fullUrl, system_time_key, metrics->system_time);
    url_addi64val(&fullUrl, user_time_key, metrics->user_time);
    url_addintval(&fullUrl, adb_liveness_key, metrics->adb_liveness);
    url_addi64val(&fullUrl, wallclock_time_key, metrics->wallclock_time);
    url_addi64val(&fullUrl, user_actions_key, metrics->user_actions);
    url_addintval(&fullUrl, exit_started_key, metrics->exit_started ? 1 : 0);
    url_addintval(&fullUrl, renderer_key, metrics->renderer);
    if (metrics->guest_gpu_enabled > 0) {
        url_addstrval(&fullUrl, guest_gl_vendor_key, metrics->guest_gl_vendor);
        url_addstrval(
                &fullUrl, guest_gl_renderer_key, metrics->guest_gl_renderer);
        url_addstrval(
                &fullUrl, guest_gl_version_key, metrics->guest_gl_version);
    }

    // Host GPU information should go last.
    // If any truncation due to too-long URLs happens,
    // we would like to prioritize the key-values above.
    URL_GPU_FORMAT(&fullUrl, gpu0);
    URL_GPU_FORMAT(&fullUrl, gpu1);
    URL_GPU_FORMAT(&fullUrl, gpu2);
    URL_GPU_FORMAT(&fullUrl, gpu3);

    const int result = fullUrl.size();
    *ptr = android::base::strDup(fullUrl);
    return result;
}

/* typedef'ed to: androidMetricsUploaderFunction */
bool androidMetrics_uploadMetricsToolbar(const AndroidMetrics* metrics) {
    static const char toolbar_url[] = "https://tools.google.com/service/update";
    bool success = true;

    char* formatted_url = NULL;
    if (formatToolbarGetUrl(&formatted_url, toolbar_url, metrics) < 0) {
        mwarning("Failed to allocate memory for a request");
        return false;
    }

    char* error = nullptr;
    if (!curl_download_null(formatted_url, nullptr, true, &error)) {
        mwarning("Can't upload usage metrics: %s", error);
        ::free(error);
        success = false;
    }
    free(formatted_url);
    return success;
}
