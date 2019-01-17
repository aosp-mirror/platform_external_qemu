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

#include "android/base/files/PathUtils.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/testing/TestTempDir.h"
#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

#include <memory>

using namespace android::base;

namespace android {
namespace perflogger {

class BenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        mTestOutputDir =
        pj(TestSystem::getProgramDirectoryFromPlatform(),
           "perfgate");
    }
    std::string mTestOutputDir;
};

// Tests construction/deconstruction
TEST_F(BenchmarkTest, Basic) {
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
        mTestOutputDir,
        "testBenchmark",
        "testBenchmarkProject",
        "testBenchmarkDescription",
        metadata);

    EXPECT_TRUE(benchCustomDir.getCustomOutputDir());
    EXPECT_EQ(mTestOutputDir, *(benchCustomDir.getCustomOutputDir()));
    EXPECT_EQ("testBenchmark", benchCustomDir.getName());
    EXPECT_EQ("testBenchmarkProject", benchCustomDir.getProjectName());
    EXPECT_EQ("testBenchmarkDescription", benchCustomDir.getDescription());
}

// Tests that a benchmark metric can be logged.
TEST_F(BenchmarkTest, File) {
    std::string metricName = "cpu_test";
    std::string expectedPath = pj(mTestOutputDir, metricName + ".json");

    {
        Benchmark::Metadata metadata;
        Benchmark bench(mTestOutputDir, "testBenchmark", "testBenchmarkProject",
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

} // namespace perflogger
} // namespace android