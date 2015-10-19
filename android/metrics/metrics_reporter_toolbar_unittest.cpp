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
    char* formatted_url = NULL;
    AndroidMetrics metrics;
    static const char kExpected[] =
            "https://tools.google.com/service/update?"
            "as%3Dandroidsdk_emu_crash%26version%3Dunknown"
            "%26id%3D00000000-0000-0000-0000-000000000000"
            "%26guest_arch%3Dunknown%26exf%3D1%26opengl_alive%3D0%26system_time%3D0"
            "%26user_time%3D0";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    EXPECT_EQ(kExpectedLen,
              formatToolbarGetUrl(&formatted_url, mToolbarUrl, &metrics));
    EXPECT_STREQ(kExpected, formatted_url);
    androidMetrics_fini(&metrics);
    free(formatted_url);
}

TEST_F(MetricsReporterToolbarTest, cleanRun) {
    char* formatted_url = NULL;
    AndroidMetrics metrics;
    static const char kExpected[] =
            "https://tools.google.com/service/update?"
            "as%3Dandroidsdk_emu_crash%26version%3Dstandalone"
            "%26id%3D00000000-0000-0000-0000-000000000000%26guest_arch%3Dx86_64"
            "%26exf%3D0%26opengl_alive%3D1%26system_time%3D1170%26user_time%3D220";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.emulator_version, "standalone");
    ANDROID_METRICS_STRASSIGN(metrics.guest_arch, "x86_64");
    metrics.guest_gpu_enabled = 0;
    metrics.opengl_alive = 1;
    metrics.tick = 1;
    metrics.system_time = 1170;
    metrics.user_time = 220;
    metrics.is_dirty = 0;
    metrics.num_failed_reports = 7;

    EXPECT_EQ(kExpectedLen,
              formatToolbarGetUrl(&formatted_url, mToolbarUrl, &metrics));
    EXPECT_STREQ(kExpected, formatted_url);
    androidMetrics_fini(&metrics);
    free(formatted_url);
}

TEST_F(MetricsReporterToolbarTest, dirtyRun) {
    char* formatted_url = NULL;
    AndroidMetrics metrics;
    static const char kExpected[] =
            "https://tools.google.com/service/update?"
            "as%3Dandroidsdk_emu_crash%26version%3Dstandalone"
            "%26id%3D00000000-0000-0000-0000-000000000000%26guest_arch%3Dx86_64"
            "%26exf%3D1%26opengl_alive%3D1%26system_time%3D1080%26user_time%3D180";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.emulator_version, "standalone");
    ANDROID_METRICS_STRASSIGN(metrics.guest_arch, "x86_64");
    metrics.guest_gpu_enabled = 0;
    metrics.opengl_alive = 1;
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

TEST_F(MetricsReporterToolbarTest, openGLErrorRun) {
    char* formatted_url = NULL;
    AndroidMetrics metrics;
    static const char kExpected[] =
            "https://tools.google.com/service/update?"
            "as%3Dandroidsdk_emu_crash%26version%3Dstandalone"
            "%26id%3D00000000-0000-0000-0000-000000000000%26guest_arch%3Dx86_64"
            "%26exf%3D1%26opengl_alive%3D0%26system_time%3D1080%26user_time%3D180";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.emulator_version, "standalone");
    ANDROID_METRICS_STRASSIGN(metrics.guest_arch, "x86_64");
    metrics.guest_gpu_enabled = 0;
    metrics.opengl_alive = 0;
    metrics.tick = 1;
    metrics.system_time = 1080;
    metrics.user_time = 180;
    metrics.is_dirty = 1;
    metrics.num_failed_reports = 9;

    EXPECT_EQ(kExpectedLen,
              formatToolbarGetUrl(&formatted_url, mToolbarUrl, &metrics));
    EXPECT_STREQ(kExpected, formatted_url);
    androidMetrics_fini(&metrics);
    free(formatted_url);
}

TEST_F(MetricsReporterToolbarTest, gpuStrings) {
    char* formatted_url = NULL;
    AndroidMetrics metrics;
    static const char kExpected[] =
            "https://tools.google.com/service/update?as%3Dandroidsdk_emu_crash"
            "%26version%3Dstandalone%26id%3D"
            "00000000-0000-0000-0000-000000000000%26guest_arch%3Dx86_64%26exf"
            "%3D0%26opengl_alive%3D1%26system_time%3D1170%26user_time%3D220%26ggl_vendor%3D"
            "Some_Vendor%26ggl_renderer%3D%26ggl_version%3D1%20.%200";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.emulator_version, "standalone");
    ANDROID_METRICS_STRASSIGN(metrics.guest_arch, "x86_64");
    metrics.tick = 1;
    metrics.system_time = 1170;
    metrics.user_time = 220;
    metrics.is_dirty = 0;
    metrics.num_failed_reports = 7;
    metrics.guest_gpu_enabled = 1;
    metrics.opengl_alive = 1;
    ANDROID_METRICS_STRASSIGN(metrics.guest_gl_vendor, "Some_Vendor");
    ANDROID_METRICS_STRASSIGN(metrics.guest_gl_renderer, "");
    ANDROID_METRICS_STRASSIGN(metrics.guest_gl_version, "1 . 0");

    EXPECT_EQ(kExpectedLen,
              formatToolbarGetUrl(&formatted_url, mToolbarUrl, &metrics));
    EXPECT_STREQ(kExpected, formatted_url);
    androidMetrics_fini(&metrics);
    free(formatted_url);
}

}  // namespace
