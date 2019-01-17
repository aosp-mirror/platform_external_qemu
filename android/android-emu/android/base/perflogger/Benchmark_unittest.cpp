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

#include "android/base/misc/FileUtils.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <memory>

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

} // namespace perflogger
} // namespace android