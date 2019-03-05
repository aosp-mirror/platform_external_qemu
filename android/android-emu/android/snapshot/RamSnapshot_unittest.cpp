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
    const int numTrials = 10;
    const float zeroPageChance = 0.5;

    for (int i = 0; i < numTrials; i++) {
        auto testRam = generateRandomRam(numPages, zeroPageChance, i);

        auto blockForTest =
            makeRam("testRam", testRam.data(), (int64_t)testRam.size());

        saveRamSingleBlock(RamSaver::Flags::Compress,
                           blockForTest,
                           ramPath);

        TestRamBuffer testRamOut(numPages * kTestingPageSize);

        auto blockForTestOutput =
            makeRam("testRam", testRamOut.data(), (int64_t)testRamOut.size());

        loadRamSingleBlock(blockForTestOutput, ramPath);

        EXPECT_EQ(testRam, testRamOut);
    }
}

TEST_F(RamSnapshotTest, IncrementalSaveRandomNoChanges) {
    std::string ramPath = mTempDir->makeSubPath("ram.bin");

    const int numPages = 100;
    const int numTrials = 10;
    const float zeroPageChance = 0.5;

    for (int i = 0; i < numTrials; i++) {
        auto testRam = generateRandomRam(numPages, zeroPageChance, i);

        auto blockForTest =
            makeRam("testRam", testRam.data(), (int64_t)testRam.size());

        saveRamSingleBlock(RamSaver::Flags::Compress,
                           blockForTest,
                           ramPath);

        incrementalSaveSingleBlock(RamSaver::Flags::Compress,
                                   blockForTest,
                                   blockForTest,
                                   ramPath);

        TestRamBuffer testRamOut(numPages * kTestingPageSize);

        auto blockForTestOutput =
            makeRam("testRam", testRamOut.data(), (int64_t)testRamOut.size());

        loadRamSingleBlock(blockForTestOutput, ramPath);

        EXPECT_EQ(testRam, testRamOut);
    }
}

TEST_F(RamSnapshotTest, IncrementalSaveRandom) {
    std::string ramPath = mTempDir->makeSubPath("ram.bin");

    const int numPages = 100;
    const int numTrials = 10;
    const float noChangeChance = 0.5;
    const float zeroPageChance = 0.5;

    for (int i = 0; i < numTrials; i++) {
        auto ramToLoad = generateRandomRam(numPages, zeroPageChance, i);
        auto ramToSave = ramToLoad;

        auto blockForLoad =
            makeRam("testRam", ramToLoad.data(), (int64_t)ramToLoad.size());

        saveRamSingleBlock(RamSaver::Flags::Compress,
                           {"testRam", 0x0, ramToLoad.data(), (int64_t)ramToLoad.size(),
                            kTestingPageSize},
                           ramPath);

        randomMutateRam(ramToSave, noChangeChance, zeroPageChance, i);

        auto blockForSave =
            makeRam("testRam", ramToSave.data(), (int64_t)ramToSave.size());

        incrementalSaveSingleBlock(RamSaver::Flags::Compress,
                                   blockForLoad,
                                   blockForSave,
                                   ramPath);

        TestRamBuffer testRamOut(numPages * kTestingPageSize);
        auto blockForTestOutput =
            makeRam("testRam", testRamOut.data(), (int64_t)testRamOut.size());

        loadRamSingleBlock(blockForTestOutput, ramPath);

        EXPECT_EQ(ramToSave, testRamOut);
    }
}

TEST_F(RamSnapshotTest, IncrementalSaveRandomMultiStep) {
    std::string ramPath = mTempDir->makeSubPath("ram.bin");

    const int numPages = 10;
    const int numTrials = 2;
    const int steps = 5;
    const float noChangeChance = 0.75;
    const float zeroPageChance = 0.5;

    for (int i = 0; i < numTrials; i++) {
        auto ramToLoad = generateRandomRam(numPages, zeroPageChance, i);
        auto ramToSave = ramToLoad;

        saveRamSingleBlock(RamSaver::Flags::Compress,
                           {"testRam", 0x0, ramToLoad.data(), (int64_t)ramToLoad.size(),
                            kTestingPageSize},
                           ramPath);

        for (int j = 0; j < steps; j++) {
            randomMutateRam(ramToSave, noChangeChance, zeroPageChance, i * steps + j);

            incrementalSaveSingleBlock(RamSaver::Flags::Compress,
                    {"testRam", 0x0, ramToLoad.data(),
                    (int64_t)ramToLoad.size(), kTestingPageSize},
                    {"testRam", 0x0, ramToSave.data(),
                    (int64_t)ramToSave.size(), kTestingPageSize},
                    ramPath);

            TestRamBuffer testRamOut(numPages * kTestingPageSize);
            loadRamSingleBlock({"testRam", 0x0, testRamOut.data(),
                                (int64_t)testRamOut.size(), kTestingPageSize},
                               ramPath);

            EXPECT_EQ(ramToSave, testRamOut);
        }
    }
}

}  // namespace snapshot
}  // namespace android
