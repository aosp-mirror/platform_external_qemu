// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Must be included before anything that might include stdint.
#include "android/base/Limits.h"

#include "android/metrics/metrics_reporter_toolbar.h"
#include "android/metrics/internal/metrics_reporter_toolbar_internal.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

namespace {

class MetricsReporterToolbarTest : public testing::Test {
public:
    MetricsReporterToolbarTest() : mTestSystem("", 32) {}

    virtual void SetUp() {
        // Set the envvar for studio preferences file to a known empty location.
        // This ensures that our tests are hermetic w.r.t. studio prefrences.
        mTestSystem.envSet("ANDROID_STUDIO_PREFERENCES",
                           mTestSystem.getTempRoot()->path());
    }

protected:
    static const char* mToolbarUrl;

private:
    android::base::TestSystem mTestSystem;
};

// static
const char* MetricsReporterToolbarTest::mToolbarUrl =
        "https://tools.google.com/service/update";

TEST_F(MetricsReporterToolbarTest, defaultMetrics) {
    char* formatted_url;
    AndroidMetrics metrics;
    static const char kExpected[] =
            "https://tools.google.com/service/update?"
            "as=androidsdk_emu_crash&version=unknown"
            "&id=00000000-0000-0000-0000-000000000000&guest_arch=unknown"
            "&exf=1&system_time=0&user_time=0";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ASSERT_EQ(kExpectedLen,
              formatToolbarGetUrl(&formatted_url, mToolbarUrl, &metrics));
    ASSERT_STREQ(kExpected, formatted_url);
    androidMetrics_fini(&metrics);
    free(formatted_url);
}

TEST_F(MetricsReporterToolbarTest, cleanRun) {
    char* formatted_url;
    AndroidMetrics metrics;
    static const char kExpected[] =
            "https://tools.google.com/service/update?"
            "as=androidsdk_emu_crash&version=standalone"
            "&id=00000000-0000-0000-0000-000000000000&guest_arch=x86_64"
            "&exf=0&system_time=1170&user_time=220";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.emulator_version, "standalone");
    ANDROID_METRICS_STRASSIGN(metrics.guest_arch, "x86_64");
    metrics.guest_gpu_enabled = 1;
    metrics.tick = 1;
    metrics.system_time = 1170;
    metrics.user_time = 220;
    metrics.is_dirty = 0;
    metrics.num_failed_reports = 7;

    ASSERT_EQ(kExpectedLen,
              formatToolbarGetUrl(&formatted_url, mToolbarUrl, &metrics));
    ASSERT_STREQ(kExpected, formatted_url);
    androidMetrics_fini(&metrics);
    free(formatted_url);
}

TEST_F(MetricsReporterToolbarTest, dirtyRun) {
    char* formatted_url;
    AndroidMetrics metrics;
    static const char kExpected[] =
            "https://tools.google.com/service/update?"
            "as=androidsdk_emu_crash&version=standalone"
            "&id=00000000-0000-0000-0000-000000000000&guest_arch=x86_64"
            "&exf=1&system_time=1080&user_time=180";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.emulator_version, "standalone");
    ANDROID_METRICS_STRASSIGN(metrics.guest_arch, "x86_64");
    metrics.guest_gpu_enabled = 1;
    metrics.tick = 1;
    metrics.system_time = 1080;
    metrics.user_time = 180;
    metrics.is_dirty = 1;
    metrics.num_failed_reports = 9;

    ASSERT_EQ(kExpectedLen,
              formatToolbarGetUrl(&formatted_url, mToolbarUrl, &metrics));
    ASSERT_STREQ(kExpected, formatted_url);
    androidMetrics_fini(&metrics);
    free(formatted_url);
}

}  // namespace
