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
#include "android/snapshot/RamLoader.h"
#include "android/snapshot/RamSaver.h"

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
using android::snapshot::RamBlock;
using android::snapshot::RamLoader;
using android::snapshot::RamSaver;

static constexpr int kPageSize = 4096;
using TestRam = AlignedBuf<uint8_t, kPageSize>;

class RamSnapshotTest : public ::testing::Test {
protected:
    void SetUp() override {
        mTempDir.reset(new TestTempDir("ramsnapshottest"));
    }

    void TearDown() override { mTempDir.reset(); }

    void saveRam(uint8_t* buffer,
                 size_t size,
                 RamSaver::Flags flags,
                 StringView filename) {
        const RamBlock block = {
                "testRam", 0x0, buffer, (int64_t)size, kPageSize,
        };

        RamSaver s(filename.c_str(), flags, nullptr, true);

        s.registerBlock(block);
        s.savePage(0x0, 0x0000, kPageSize);
        s.join();
    }

    void loadRam(StringView filename, uint8_t* buffer, size_t size) {
        auto ram = fopen(filename.c_str(), "rb");

        EXPECT_TRUE(ram);

        RamLoader::RamBlockStructure emptyRamBlockStructure = {};

        // Disallow on-demand load for now.
        RamLoader ramLoader(StdioStream(ram, StdioStream::kOwner),
                            RamLoader::Flags::None, emptyRamBlockStructure);

        const RamBlock block = {
                "testRam", 0x0, buffer, (int64_t)size, kPageSize,
        };

        ramLoader.registerBlock(block);

        ramLoader.start(false);
        ramLoader.join();
    }

    TestRam randomRam(size_t numPages, float zeroPageChance) {
        std::default_random_engine generator;
        // Use a consistent seed value to avoid flakes
        generator.seed(0);

        // Distributions for the random patterns and zero pages
        std::uniform_int_distribution<char> patternDistribution(0, 255);
        std::bernoulli_distribution zeroPageDistribution(zeroPageChance);

        TestRam res(numPages * kPageSize);

        uint8_t* ram = res.data();

        for (size_t i = 0; i < numPages; ++i) {
            uint8_t* currentPage = ram + i * kPageSize;
            if (zeroPageDistribution(generator)) {
                memset(currentPage, 0x0, kPageSize);
            } else {
                char pattern = patternDistribution(generator);
                memset(ram + i * kPageSize, pattern, kPageSize);
            }
        }

        return res;
    }

    std::unique_ptr<TestTempDir> mTempDir;
};

TEST_F(RamSnapshotTest, SimpleRandom) {
    std::string ramPath = mTempDir->makeSubPath("ram.bin");

    const int numPages = 200;
    const float zeroPageChance = 0.5;

    auto testRam = randomRam(numPages, zeroPageChance);

    saveRam(testRam.data(), testRam.size(), RamSaver::Flags::None, ramPath);

    TestRam testRamOut(numPages * kPageSize);
    loadRam(ramPath, testRamOut.data(), testRamOut.size());

    for (int i = 0; i < numPages; i++) {
        EXPECT_EQ(testRamOut[i], testRam[i]);
    }
}
