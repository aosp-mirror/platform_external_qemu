// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/AlignedBuf.h"
#include "android/base/StringView.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/testing/TestTempDir.h"
#include "android/snapshot/RamSnapshotTesting.h"

#include <gtest/gtest.h>

#include <memory>
#include <random>
#include <vector>

using android::AlignedBuf;
using android::base::PathUtils;
using android::base::StdioStream;
using android::base::StringView;
using android::base::System;
using android::base::TestTempDir;

namespace android {
namespace snapshot {

class RamSnapshotTest : public ::testing::Test {
protected:
    void SetUp() override {
        mTempDir.reset(new TestTempDir("ramsnapshottest"));
    }

    void TearDown() override { mTempDir.reset(); }

    std::unique_ptr<TestTempDir> mTempDir;
};

TEST_F(RamSnapshotTest, SimpleRandom) {
    std::string ramPath = mTempDir->makeSubPath("ram.bin");

    const int numPages = 100;
    const float zeroPageChance = 0.5;

    auto testRam = generateRandomRam(numPages, zeroPageChance);

    saveRamSingleBlock(RamSaver::Flags::Compress,
                       {"testRam", 0x0, testRam.data(), (int64_t)testRam.size(),
                        kTestingPageSize},
                       ramPath);

    TestRamBuffer testRamOut(numPages * kTestingPageSize);
    loadRamSingleBlock({"testRam", 0x0, testRamOut.data(),
                        (int64_t)testRamOut.size(), kTestingPageSize},
                       ramPath);

    for (int i = 0; i < numPages; i++) {
        EXPECT_EQ(testRamOut[i], testRam[i]);
    }
}

}  // namespace snapshot
}  // namespace android
