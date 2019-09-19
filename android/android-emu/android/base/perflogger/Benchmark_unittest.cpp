// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/base/perflogger/Benchmark.h"

#include <gtest/gtest-message.h>               // for Message
#include <gtest/gtest-test-part.h>             // for TestPartResult
#include <stdio.h>                             // for printf
#include <memory>                              // for unique_ptr
#include <string>                              // for operator+, basic_string

#include "android/base/files/PathUtils.h"      // for pj
#include "android/base/misc/FileUtils.h"       // for readFileIntoString
#include "android/base/perflogger/Metric.h"    // for Metric
#include "android/base/testing/TestTempDir.h"  // for TestTempDir
#include "android/base/Log.h"                  // for base
#include "gtest/gtest_pred_impl.h"             // for AssertionResult, EXPEC...

using namespace android::base;

namespace android {
namespace perflogger {

class BenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override { mTempDir.reset(new TestTempDir("benchmarktest")); }
    void TearDown() override { mTempDir.reset(); }
    std::unique_ptr<TestTempDir> mTempDir;
};

// Tests construction/deconstruction
TEST_F(BenchmarkTest, Basic) {
    std::string testDir = mTempDir->makeSubPath("testDir");

    Benchmark::Metadata metadata;

    Benchmark benchNoCustomDir(
        "testBenchmark",
        "testBenchmarkProject",
        "testBenchmarkDescription",
        metadata);

    EXPECT_FALSE(benchNoCustomDir.getCustomOutputDir());
    EXPECT_EQ("testBenchmark", benchNoCustomDir.getName());
    EXPECT_EQ("testBenchmarkProject", benchNoCustomDir.getProjectName());
    EXPECT_EQ("testBenchmarkDescription", benchNoCustomDir.getDescription());

    Benchmark benchCustomDir(
        testDir,
        "testBenchmark",
        "testBenchmarkProject",
        "testBenchmarkDescription",
        metadata);

    EXPECT_TRUE(benchCustomDir.getCustomOutputDir());
    EXPECT_EQ(testDir, *(benchCustomDir.getCustomOutputDir()));
    EXPECT_EQ("testBenchmark", benchCustomDir.getName());
    EXPECT_EQ("testBenchmarkProject", benchCustomDir.getProjectName());
    EXPECT_EQ("testBenchmarkDescription", benchCustomDir.getDescription());
}

// Tests that a benchmark metric can be logged.
TEST_F(BenchmarkTest, File) {
    std::string metricName = "benchmark_test";

    std::string testDir = mTempDir->makeSubPath("testDir");
    std::string expectedPath = pj(testDir, metricName + ".json");

    {
        Benchmark::Metadata metadata;
        Benchmark bench(testDir, "testBenchmark", "testBenchmarkProject",
                        "testBenchmarkDescription", metadata);

        bench.log(metricName, 12345);
    }

    const auto fileContents = android::readFileIntoString(expectedPath);
    EXPECT_TRUE(fileContents);
    EXPECT_GT(fileContents->size(), 0);

    // TODO: test validity of resulting JSON
    // For now, FYI it
    printf("Resulting JSON: [%s]\n", fileContents->c_str());
}

// Logs an actual metric JSON to the perfgate folder, whether that is
// DIST_DIR or what.
TEST_F(BenchmarkTest, PerfgateSmokeTest) {
    std::string metricName = "perfgate_test";

    std::string expectedPath =
        pj(Metric::getPreferredOutputDirectory(), metricName + ".json");

    {
        Benchmark::Metadata metadata;
        Benchmark bench("testBenchmark", "testBenchmarkProject",
                        "testBenchmarkDescription", metadata);

        bench.log(metricName, 1);
    }

    const auto fileContents = android::readFileIntoString(expectedPath);
    EXPECT_TRUE(fileContents);
    EXPECT_GT(fileContents->size(), 0);
}

} // namespace perflogger
} // namespace android
