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

#include "android/metrics/metrics_reporter.h"
#include "android/metrics/internal/metrics_reporter_internal.h"

#include <gtest/gtest.h>
#include <fstream>
#include <cstdlib>

#include "android/base/String.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/testing/TestTempDir.h"

namespace {

using android::base::TestTempDir;
using android::base::ScopedPtr;
using android::base::String;
using std::ofstream;

class MetricsReporterTest : public testing::Test {
public:
    virtual void SetUp() {
        // Yes, I know this means we can't run these tests in parallel. But
        // seriously, a whole lot of things in the code being tested will break
        // as well.
        upload_calls_count_ = 0;
        upload_success_count_ = 0;
        temp_dir_.reset(new TestTempDir("MetricsReporterTest"));
        ASSERT_FALSE(NULL == temp_dir_->path());
        androidMetrics_moduleInit(temp_dir_->path());
    }

    virtual void TearDown() {
        // It's safe to call androidMetrics_seal repeatedly.
        androidMetrics_seal();
        androidMetrics_moduleFini();
        temp_dir_.reset(NULL);
        temp_dir_.reset(NULL);
    }

    void createValidFile(const char* filename) {
        ofstream ini_file;
        ini_file.open(temp_dir_->makeSubPath(filename).c_str());
        ini_file << "tick = 3";
        ini_file.close();
    }

    static ABool testUploader(const AndroidMetrics*) {
        bool should_succeed = (upload_success_count_ > 0);
        ++upload_calls_count_;
        if (should_succeed) {
            --upload_success_count_;
            return 1;
        }
        return 0;
    }

protected:
    static int upload_calls_count_;
    static int upload_success_count_;
    ScopedPtr<TestTempDir> temp_dir_;
};

// static
int MetricsReporterTest::upload_calls_count_ = 0;
int MetricsReporterTest::upload_success_count_ = 0;

void assertAndroidMetricsEq(const AndroidMetrics& lhs,
                            const AndroidMetrics& rhs) {
// Use the magic of macros to compare fields.
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d) ASSERT_EQ(lhs.n, rhs.n);
#define METRICS_STRING(n, s, d) ASSERT_EQ(0, strcmp(lhs.n, rhs.n));
#define METRICS_DURATION(n, s, d) ASSERT_EQ(lhs.n, rhs.n);

#include "android/metrics/metrics_fields.h"
}

// Run through the drop metrics flow, to test that it doesn't blow up, and that
// write-read preserved data.
TEST_F(MetricsReporterTest, simpleWriteRead) {
    const char* new_arch = "is_known_test";
    AndroidMetrics metrics_out, metrics_in;
    int tick_count = 5;

    androidMetrics_init(&metrics_out);
    metrics_out.guest_gpu_enabled = 1;
    ANDROID_METRICS_STRASSIGN(metrics_out.guest_arch, new_arch);
    ASSERT_TRUE(androidMetrics_write(&metrics_out));

    androidMetrics_init(&metrics_in);
    ASSERT_TRUE(androidMetrics_read(&metrics_in));
    assertAndroidMetricsEq(metrics_in, metrics_out);
    androidMetrics_fini(&metrics_in);
    androidMetrics_fini(&metrics_out);

    for (int i = 0; i < tick_count; ++i) {
        androidMetrics_tick();
    }

    androidMetrics_init(&metrics_in);
    ASSERT_TRUE(androidMetrics_read(&metrics_in));
    ASSERT_EQ(tick_count, metrics_in.tick);
    androidMetrics_fini(&metrics_in);
}

TEST_F(MetricsReporterTest, seal) {
    const char* new_arch = "is_known_test";
    String stashed_path;
    AndroidMetrics metrics;

    androidMetrics_init(&metrics);
    ANDROID_METRICS_STRASSIGN(metrics.guest_arch, new_arch);
    ASSERT_EQ(1, metrics.is_dirty);
    ASSERT_TRUE(androidMetrics_write(&metrics));
    androidMetrics_fini(&metrics);

    stashed_path = androidMetrics_getMetricsFilePath();
    androidMetrics_seal();

    androidMetrics_init(&metrics);
    ASSERT_TRUE(androidMetrics_readPath(&metrics, stashed_path.c_str()));
    ASSERT_EQ(0, strcmp(new_arch, metrics.guest_arch));
    ASSERT_EQ(0, metrics.is_dirty);
    androidMetrics_fini(&metrics);
}

TEST_F(MetricsReporterTest, reportAllNoReports) {
    // Be more realistic, initialize our own metrics first.
    AndroidMetrics metrics;
    androidMetrics_init(&metrics);
    ASSERT_TRUE(androidMetrics_write(&metrics));
    androidMetrics_fini(&metrics);

    androidMetrics_injectUploader(&MetricsReporterTest::testUploader);
    ASSERT_TRUE(androidMetrics_tryReportAll());
    ASSERT_EQ(0, this->upload_calls_count_);
}

TEST_F(MetricsReporterTest, reportAllSuccess) {
    // Be more realistic, initialize our own metrics first.
    AndroidMetrics metrics;
    androidMetrics_init(&metrics);
    ASSERT_TRUE(androidMetrics_write(&metrics));
    androidMetrics_fini(&metrics);

    this->upload_success_count_ = 3;
    this->createValidFile("metrics/metrics.123.yogibear");
    this->createValidFile("metrics/metrics.456.yogibear");
    this->createValidFile("metrics/metrics.789.yogibear");
    androidMetrics_injectUploader(&MetricsReporterTest::testUploader);
    ASSERT_TRUE(androidMetrics_tryReportAll());
    ASSERT_EQ(3, this->upload_calls_count_);
    ASSERT_EQ(0, this->upload_success_count_);
}

TEST_F(MetricsReporterTest, reportAllPartialFailure) {
    // Be more realistic, initialize our own metrics first.
    AndroidMetrics metrics;
    androidMetrics_init(&metrics);
    ASSERT_TRUE(androidMetrics_write(&metrics));
    androidMetrics_fini(&metrics);

    // Only succeed the first two times.
    this->upload_success_count_ = 2;
    this->createValidFile("metrics/metrics.123.yogibear");
    this->createValidFile("metrics/metrics.456.yogibear");
    this->createValidFile("metrics/metrics.789.yogibear");
    androidMetrics_injectUploader(&MetricsReporterTest::testUploader);
    ASSERT_FALSE(androidMetrics_tryReportAll());
    ASSERT_EQ(3, this->upload_calls_count_);
    ASSERT_EQ(0, this->upload_success_count_);
}

}  // namespace
