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
            "as=androidsdk_emu_crash&version=unknown&core_version=unknown"
            "&update_channel=0"
            "&os=unknown&id=00000000-0000-0000-0000-000000000000"
            "&guest_arch=unknown&guest_api_level=-1&exf=1&opengl_alive=0"
            "&system_time=0&user_time=0&adb_liveness=0&wall_time=0"
            "&user_actions=0"
            "&exit_started=0"
            "&renderer=0"
            "&gpu0_make=unknown"
            "&gpu0_model=unknown"
            "&gpu0_device_id=unknown"
            "&gpu0_revision_id=unknown"
            "&gpu0_version=unknown"
            "&gpu0_renderer=unknown"
            "&gpu1_make=unknown"
            "&gpu1_model=unknown"
            "&gpu1_device_id=unknown"
            "&gpu1_revision_id=unknown"
            "&gpu1_version=unknown"
            "&gpu1_renderer=unknown"
            "&gpu2_make=unknown"
            "&gpu2_model=unknown"
            "&gpu2_device_id=unknown"
            "&gpu2_revision_id=unknown"
            "&gpu2_version=unknown"
            "&gpu2_renderer=unknown"
            "&gpu3_make=unknown"
            "&gpu3_model=unknown"
            "&gpu3_device_id=unknown"
            "&gpu3_revision_id=unknown"
            "&gpu3_version=unknown"
            "&gpu3_renderer=unknown";
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
            "as=androidsdk_emu_crash&version=standalone&core_version=qemu15"
            "&update_channel=0"
            "&os=lynx&id=00000000-0000-0000-0000-000000000000&guest_arch=x86_64"
            "&guest_api_level=100500&exf=0&opengl_alive=1&system_time=1170"
            "&user_time=220&adb_liveness=0&wall_time=10&user_actions=10"
            "&exit_started=1"
            "&renderer=0"
            "&gpu0_make=unknown"
            "&gpu0_model=unknown"
            "&gpu0_device_id=unknown"
            "&gpu0_revision_id=unknown"
            "&gpu0_version=unknown"
            "&gpu0_renderer=unknown"
            "&gpu1_make=unknown"
            "&gpu1_model=unknown"
            "&gpu1_device_id=unknown"
            "&gpu1_revision_id=unknown"
            "&gpu1_version=unknown"
            "&gpu1_renderer=unknown"
            "&gpu2_make=unknown"
            "&gpu2_model=unknown"
            "&gpu2_device_id=unknown"
            "&gpu2_revision_id=unknown"
            "&gpu2_version=unknown"
            "&gpu2_renderer=unknown"
            "&gpu3_make=unknown"
            "&gpu3_model=unknown"
            "&gpu3_device_id=unknown"
            "&gpu3_revision_id=unknown"
            "&gpu3_version=unknown"
            "&gpu3_renderer=unknown";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.emulator_version, "standalone");
    ANDROID_METRICS_STRASSIGN(metrics.core_version, "qemu15");
    ANDROID_METRICS_STRASSIGN(metrics.host_os_type, "lynx");
    ANDROID_METRICS_STRASSIGN(metrics.guest_arch, "x86_64");
    metrics.guest_api_level = 100500;
    metrics.guest_gpu_enabled = 0;
    metrics.opengl_alive = 1;
    metrics.tick = 1;
    metrics.system_time = 1170;
    metrics.user_time = 220;
    metrics.wallclock_time = 10;
    metrics.user_actions = 10;
    metrics.is_dirty = 0;
    metrics.num_failed_reports = 7;
    metrics.exit_started = 1;

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
            "as=androidsdk_emu_crash&version=standalone&core_version=unknown"
            "&update_channel=0"
            "&os=lynx&id=00000000-0000-0000-0000-000000000000&guest_arch=x86_64"
            "&guest_api_level=-1"
            "&exf=1&opengl_alive=1&system_time=1080&user_time=180"
            "&adb_liveness=0&wall_time=101&user_actions=432&exit_started=0"
            "&renderer=0"
            "&gpu0_make=unknown"
            "&gpu0_model=unknown"
            "&gpu0_device_id=unknown"
            "&gpu0_revision_id=unknown"
            "&gpu0_version=unknown"
            "&gpu0_renderer=unknown"
            "&gpu1_make=unknown"
            "&gpu1_model=unknown"
            "&gpu1_device_id=unknown"
            "&gpu1_revision_id=unknown"
            "&gpu1_version=unknown"
            "&gpu1_renderer=unknown"
            "&gpu2_make=unknown"
            "&gpu2_model=unknown"
            "&gpu2_device_id=unknown"
            "&gpu2_revision_id=unknown"
            "&gpu2_version=unknown"
            "&gpu2_renderer=unknown"
            "&gpu3_make=unknown"
            "&gpu3_model=unknown"
            "&gpu3_device_id=unknown"
            "&gpu3_revision_id=unknown"
            "&gpu3_version=unknown"
            "&gpu3_renderer=unknown";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.emulator_version, "standalone");
    ANDROID_METRICS_STRASSIGN(metrics.host_os_type, "lynx");
    ANDROID_METRICS_STRASSIGN(metrics.guest_arch, "x86_64");
    metrics.guest_gpu_enabled = 0;
    metrics.opengl_alive = 1;
    metrics.tick = 1;
    metrics.system_time = 1080;
    metrics.user_time = 180;
    metrics.wallclock_time = 101;
    metrics.user_actions = 432;
    metrics.is_dirty = 1;
    metrics.num_failed_reports = 9;
    metrics.exit_started = 0;

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
            "as=androidsdk_emu_crash&version=standalone&core_version=unknown"
            "&update_channel=0"
            "&os=lynx&id=00000000-0000-0000-0000-000000000000&guest_arch=x86_64"
            "&guest_api_level=-1"
            "&exf=1&opengl_alive=0&system_time=1080&user_time=180"
            "&adb_liveness=0&wall_time=0&user_actions=0&exit_started=0"
            "&renderer=0"
            "&gpu0_make=unknown"
            "&gpu0_model=unknown"
            "&gpu0_device_id=unknown"
            "&gpu0_revision_id=unknown"
            "&gpu0_version=unknown"
            "&gpu0_renderer=unknown"
            "&gpu1_make=unknown"
            "&gpu1_model=unknown"
            "&gpu1_device_id=unknown"
            "&gpu1_revision_id=unknown"
            "&gpu1_version=unknown"
            "&gpu1_renderer=unknown"
            "&gpu2_make=unknown"
            "&gpu2_model=unknown"
            "&gpu2_device_id=unknown"
            "&gpu2_revision_id=unknown"
            "&gpu2_version=unknown"
            "&gpu2_renderer=unknown"
            "&gpu3_make=unknown"
            "&gpu3_model=unknown"
            "&gpu3_device_id=unknown"
            "&gpu3_revision_id=unknown"
            "&gpu3_version=unknown"
            "&gpu3_renderer=unknown";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.emulator_version, "standalone");
    ANDROID_METRICS_STRASSIGN(metrics.host_os_type, "lynx");
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

TEST_F(MetricsReporterToolbarTest, guestGpuStrings) {
    char* formatted_url = NULL;
    AndroidMetrics metrics;
    static const char kExpected[] =
            "https://tools.google.com/service/update?as=androidsdk_emu_crash"
            "&version=standalone&core_version=unknown"
            "&update_channel=0"
            "&os=lynx&id=00000000-0000-0000-0000-000000000000&guest_arch=x86_64"
            "&guest_api_level=-1"
            "&exf=0&opengl_alive=1&system_time=1170&user_time=220"
            "&adb_liveness=0&wall_time=0&user_actions=0&exit_started=0"
            "&renderer=0"
            "&ggl_vendor=Some_Vendor&ggl_renderer=&ggl_version=1%20.%200"
            "&gpu0_make=unknown"
            "&gpu0_model=unknown"
            "&gpu0_device_id=unknown"
            "&gpu0_revision_id=unknown"
            "&gpu0_version=unknown"
            "&gpu0_renderer=unknown"
            "&gpu1_make=unknown"
            "&gpu1_model=unknown"
            "&gpu1_device_id=unknown"
            "&gpu1_revision_id=unknown"
            "&gpu1_version=unknown"
            "&gpu1_renderer=unknown"
            "&gpu2_make=unknown"
            "&gpu2_model=unknown"
            "&gpu2_device_id=unknown"
            "&gpu2_revision_id=unknown"
            "&gpu2_version=unknown"
            "&gpu2_renderer=unknown"
            "&gpu3_make=unknown"
            "&gpu3_model=unknown"
            "&gpu3_device_id=unknown"
            "&gpu3_revision_id=unknown"
            "&gpu3_version=unknown"
            "&gpu3_renderer=unknown";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.emulator_version, "standalone");
    ANDROID_METRICS_STRASSIGN(metrics.host_os_type, "lynx");
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

TEST_F(MetricsReporterToolbarTest, gpuStrings) {
    char* formatted_url = NULL;
    AndroidMetrics metrics;
    static const char kExpected[] =
            "https://tools.google.com/service/update?as=androidsdk_emu_crash"
            "&version=standalone&core_version=unknown"
            "&update_channel=0"
            "&os=lynx&id=00000000-0000-0000-0000-000000000000&guest_arch=x86_64"
            "&guest_api_level=-1"
            "&exf=0&opengl_alive=1&system_time=1170&user_time=220"
            "&adb_liveness=0&wall_time=0&user_actions=0&exit_started=0"
            "&renderer=0"
            "&gpu0_make=Advanced%20Micro%20Devices%2C%20Inc."
            "&gpu0_model=AMD%20Radeon%20%28TM%29%20R5%20M335"
            "&gpu0_device_id=0x0166"
            "&gpu0_revision_id=0x0166"
            "&gpu0_version=10.18.15.4281"
            "&gpu0_renderer=OpenGL%20version%20string%3A%203.0%20Mesa%2010.4.2%"
            "20%28git-%29"
            "&gpu1_make=Advanced%20Micro%20Devices%2C%20Inc."
            "&gpu1_model=AMD%20Radeon%20%28TM%29%20R5%20M335"
            "&gpu1_device_id=0x0166"
            "&gpu1_revision_id=0x0166"
            "&gpu1_version=10.18.15.4281"
            "&gpu1_renderer=OpenGL%20version%20string%3A%203.0%20Mesa%2010.4.2%"
            "20%28git-%29"
            "&gpu2_make=Advanced%20Micro%20Devices%2C%20Inc."
            "&gpu2_model=AMD%20Radeon%20%28TM%29%20R5%20M335"
            "&gpu2_device_id=0x0166"
            "&gpu2_revision_id=0x0166"
            "&gpu2_version=10.18.15.4281"
            "&gpu2_renderer=OpenGL%20version%20string%3A%203.0%20Mesa%2010.4.2%"
            "20%28git-%29"
            "&gpu3_make=Advanced%20Micro%20Devices%2C%20Inc."
            "&gpu3_model=AMD%20Radeon%20%28TM%29%20R5%20M335"
            "&gpu3_device_id=0x0166"
            "&gpu3_revision_id=0x0166"
            "&gpu3_version=10.18.15.4281"
            "&gpu3_renderer=OpenGL%20version%20string%3A%203.0%20Mesa%2010.4.2%"
            "20%28git-%29";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.emulator_version, "standalone");
    ANDROID_METRICS_STRASSIGN(metrics.host_os_type, "lynx");
    ANDROID_METRICS_STRASSIGN(metrics.guest_arch, "x86_64");
    metrics.tick = 1;
    metrics.system_time = 1170;
    metrics.user_time = 220;
    metrics.is_dirty = 0;
    metrics.num_failed_reports = 7;
    metrics.guest_gpu_enabled = 0;
    metrics.opengl_alive = 1;

    // Just make it unrealistically long---we expect
    // this case to be < 1500 chars, and even it should pass.
    ANDROID_METRICS_STRASSIGN(metrics.gpu0_make,
                              "Advanced Micro Devices, Inc.");
    ANDROID_METRICS_STRASSIGN(metrics.gpu0_model, "AMD Radeon (TM) R5 M335");
    ANDROID_METRICS_STRASSIGN(metrics.gpu0_device_id, "0x0166");
    ANDROID_METRICS_STRASSIGN(metrics.gpu0_revision_id, "0x0166");
    ANDROID_METRICS_STRASSIGN(metrics.gpu0_version, "10.18.15.4281");
    ANDROID_METRICS_STRASSIGN(metrics.gpu0_renderer,
                              "OpenGL version string: 3.0 Mesa 10.4.2 (git-)");
    ANDROID_METRICS_STRASSIGN(metrics.gpu1_make,
                              "Advanced Micro Devices, Inc.");
    ANDROID_METRICS_STRASSIGN(metrics.gpu1_model, "AMD Radeon (TM) R5 M335");
    ANDROID_METRICS_STRASSIGN(metrics.gpu1_device_id, "0x0166");
    ANDROID_METRICS_STRASSIGN(metrics.gpu1_revision_id, "0x0166");
    ANDROID_METRICS_STRASSIGN(metrics.gpu1_version, "10.18.15.4281");
    ANDROID_METRICS_STRASSIGN(metrics.gpu1_renderer,
                              "OpenGL version string: 3.0 Mesa 10.4.2 (git-)");
    ANDROID_METRICS_STRASSIGN(metrics.gpu2_make,
                              "Advanced Micro Devices, Inc.");
    ANDROID_METRICS_STRASSIGN(metrics.gpu2_model, "AMD Radeon (TM) R5 M335");
    ANDROID_METRICS_STRASSIGN(metrics.gpu2_device_id, "0x0166");
    ANDROID_METRICS_STRASSIGN(metrics.gpu2_revision_id, "0x0166");
    ANDROID_METRICS_STRASSIGN(metrics.gpu2_version, "10.18.15.4281");
    ANDROID_METRICS_STRASSIGN(metrics.gpu2_renderer,
                              "OpenGL version string: 3.0 Mesa 10.4.2 (git-)");
    ANDROID_METRICS_STRASSIGN(metrics.gpu3_make,
                              "Advanced Micro Devices, Inc.");
    ANDROID_METRICS_STRASSIGN(metrics.gpu3_model, "AMD Radeon (TM) R5 M335");
    ANDROID_METRICS_STRASSIGN(metrics.gpu3_device_id, "0x0166");
    ANDROID_METRICS_STRASSIGN(metrics.gpu3_revision_id, "0x0166");
    ANDROID_METRICS_STRASSIGN(metrics.gpu3_version, "10.18.15.4281");
    ANDROID_METRICS_STRASSIGN(metrics.gpu3_renderer,
                              "OpenGL version string: 3.0 Mesa 10.4.2 (git-)");

    EXPECT_EQ(kExpectedLen,
              formatToolbarGetUrl(&formatted_url, mToolbarUrl, &metrics));
    EXPECT_STREQ(kExpected, formatted_url);
    androidMetrics_fini(&metrics);
    free(formatted_url);
}

TEST_F(MetricsReporterToolbarTest, rendererSelection) {
    char* formatted_url = NULL;
    AndroidMetrics metrics;
    static const char kExpected[] =
            "https://tools.google.com/service/update?"
            "as=androidsdk_emu_crash&version=unknown&core_version=unknown"
            "&update_channel=0"
            "&os=unknown&id=00000000-0000-0000-0000-000000000000"
            "&guest_arch=unknown&guest_api_level=-1&exf=1&opengl_alive=0"
            "&system_time=0&user_time=0&adb_liveness=0&wall_time=0"
            "&user_actions=0"
            "&exit_started=0"
            "&renderer=1"
            "&gpu0_make=unknown"
            "&gpu0_model=unknown"
            "&gpu0_device_id=unknown"
            "&gpu0_revision_id=unknown"
            "&gpu0_version=unknown"
            "&gpu0_renderer=unknown"
            "&gpu1_make=unknown"
            "&gpu1_model=unknown"
            "&gpu1_device_id=unknown"
            "&gpu1_revision_id=unknown"
            "&gpu1_version=unknown"
            "&gpu1_renderer=unknown"
            "&gpu2_make=unknown"
            "&gpu2_model=unknown"
            "&gpu2_device_id=unknown"
            "&gpu2_revision_id=unknown"
            "&gpu2_version=unknown"
            "&gpu2_renderer=unknown"
            "&gpu3_make=unknown"
            "&gpu3_model=unknown"
            "&gpu3_device_id=unknown"
            "&gpu3_revision_id=unknown"
            "&gpu3_version=unknown"
            "&gpu3_renderer=unknown";
    static const int kExpectedLen = (int)(sizeof(kExpected) - 1);

    androidMetrics_init(&metrics);
    metrics.renderer = 1;
    EXPECT_EQ(kExpectedLen,
              formatToolbarGetUrl(&formatted_url, mToolbarUrl, &metrics));
    EXPECT_STREQ(kExpected, formatted_url);
    androidMetrics_fini(&metrics);
    free(formatted_url);
}

}  // namespace
