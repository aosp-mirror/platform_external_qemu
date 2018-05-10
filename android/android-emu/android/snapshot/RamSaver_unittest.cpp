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

#include "android/snapshot/RamSaver.h"

#include "android/base/AlignedBuf.h"
#include "android/base/StringView.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <memory>
#include <random>
#include <vector>

using android::AlignedBuf;
using android::base::StringView;
using android::base::TestTempDir;
using android::snapshot::RamBlock;
using android::snapshot::RamSaver;

static constexpr int kPageSize = 4096;
using TestRam = AlignedBuf<uint8_t, kPageSize>;

// Needed because Quickboot statically depends on Qt and there is a link
// failure if the function is not defined.
bool userSettingIsDontSaveSnapshot() {
    return false;
}

class RamSaverTest : public ::testing::Test {
protected:
    void SetUp() override { mTempDir.reset(new TestTempDir("ramsavertest")); }

    void TearDown() override { mTempDir.reset(); }

    void saveRamSnapshot(uint8_t* buffer,
                         size_t size,
                         RamSaver::Flags flags,
                         StringView filename) {
        const RamBlock block = {
                "ramSaverTestBlock", 0x0, buffer, (int64_t)size, kPageSize,
        };

        RamSaver s(filename.c_str(), flags, nullptr, true);

        s.registerBlock(block);
        s.savePage(0x0, 0x0000, kPageSize);
        s.join();
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

static void checkFileEqualToBuffer(const uint8_t* buffer,
                                   size_t size,
                                   StringView filename) {
    const auto fileContents = android::readFileIntoString(filename);

    EXPECT_TRUE(fileContents);
    EXPECT_LE(size, fileContents->size());
    for (size_t i = 0; i < size; i++) {
        EXPECT_EQ((char)buffer[i], fileContents->at(i));
    }
}

// Save 10 pages that are all zero with no compression.
TEST_F(RamSaverTest, Simple) {
    std::string ramSaverTestPath = mTempDir->makeSubPath("ram.bin");

    const int numPages = 10;

    // Create aligned buf
    TestRam testRam(numPages * kPageSize);
    uint8_t* ramAligned = testRam.data();
    memset(ramAligned, 0, numPages * kPageSize);

    saveRamSnapshot(ramAligned, testRam.size(), RamSaver::Flags::None,
                    ramSaverTestPath);

    const std::vector<uint8_t> golden = {
            0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x8,  0x0,  0x0,
            0x0,  0x2,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0xa,
            0x11, 0x72, 0x61, 0x6d, 0x53, 0x61, 0x76, 0x65, 0x72, 0x54,
            0x65, 0x73, 0x74, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x0,  0x0,
            0x0,  0xa,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
            0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    };

    checkFileEqualToBuffer(golden.data(), golden.size(), ramSaverTestPath);
}

// Save 1 nonzero page with compression.
TEST_F(RamSaverTest, SimpleNonzero) {
    std::string ramSaverTestPath = mTempDir->makeSubPath("ram.bin");

    const int numPages = 1;
    const int kPageSize = 4096;

    // Create aligned buf
    TestRam testRam(numPages * kPageSize);
    uint8_t* ramAligned = testRam.data();
    memset(ramAligned, 0xff, numPages * kPageSize);

    saveRamSnapshot(ramAligned, testRam.size(), RamSaver::Flags::Compress,
                    ramSaverTestPath);

    const std::vector<uint8_t> golden = {
            0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x22, 0x1f, 0xff,
            0x1,  0x0,  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf6, 0x50, 0xff,
            0xff, 0xff, 0xff, 0xff, 0x0,  0x0,  0x0,  0x2,  0x0,  0x0,
            0x0,  0x1,  0x0,  0x0,  0x0,  0x1,  0x11, 0x72, 0x61, 0x6d,
            0x53, 0x61, 0x76, 0x65, 0x72, 0x54, 0x65, 0x73, 0x74, 0x42,
            0x6c, 0x6f, 0x63, 0x6b, 0x0,  0x0,  0x0,  0x1,  0x1a, 0x0,
            0xa7, 0x33, 0xb8, 0x81, 0x91, 0x17, 0x52, 0xb,  0xfa, 0xeb,
            0x17, 0xde, 0xd2, 0x1a, 0x16, 0xc4, 0x0,  0x0,  0x0,  0x0,

    };

    checkFileEqualToBuffer(golden.data(), golden.size(), ramSaverTestPath);
}

// Save 100 pages with compression, some zero, some nonzero.
// Does not actually check contents; just performs saving.
TEST_F(RamSaverTest, BasicRandom) {
    const int numPages = 100;
    const float zeroPageChance = 0.5;
    std::string ramSaverTestPath = mTempDir->makeSubPath("random-ram-100.bin");

    auto testRam = randomRam(numPages, zeroPageChance);

    saveRamSnapshot(testRam.data(), testRam.size(), RamSaver::Flags::Compress,
                    ramSaverTestPath);
}
