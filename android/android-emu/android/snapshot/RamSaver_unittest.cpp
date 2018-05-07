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
#include "android/base/testing/TestTempDir.h"
#include "android/base/misc/FileUtils.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

using android::AlignedBuf;
using android::base::TestTempDir;
using android::snapshot::RamSaver;
using android::snapshot::RamBlock;

// Needed because Quickboot statically depends on Qt and there is a link
// failure if the function is not defined.
bool userSettingIsDontSaveSnapshot() {
    return false;
}

class RamSaverTest : public ::testing::Test {
protected:
    void SetUp() override {
        mTempDir.reset(new TestTempDir("ramsavertest"));
    }

    void TearDown() override {
        mTempDir.reset();
    }

    std::unique_ptr<TestTempDir> mTempDir;
};

// Save 10 pages that are all zero with no compression.
TEST_F(RamSaverTest, Simple) {
    std::string ramSaverTestPath =
        mTempDir->makeSubPath("ram.bin");

    const int numPages = 10;
    const int pageSize = 4096;

    // Create aligned buf
    AlignedBuf<uint8_t, pageSize> testRam(numPages * pageSize);
    uint8_t* ramAligned = testRam.data();
    memset(ramAligned, 0, numPages * pageSize);

    const RamBlock block = {
        "ramSaverTestBlock",
        0x0,
        ramAligned,
        numPages * pageSize,
        pageSize,
    };

    RamSaver s(ramSaverTestPath.c_str(), RamSaver::Flags::None, nullptr, true);

    s.registerBlock(block);

    s.savePage(0x0, 0x0000, pageSize);

    s.join();

    const auto fileContents = android::readFileIntoString(ramSaverTestPath);

    const std::vector<uint8_t> golden = {
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x8,
        0x0, 0x0, 0x0, 0x2,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0xa,
        0x11, 0x72, 0x61, 0x6d,
        0x53, 0x61, 0x76, 0x65,
        0x72, 0x54, 0x65, 0x73,
        0x74, 0x42, 0x6c, 0x6f,
        0x63, 0x6b, 0x0, 0x0,
        0x0, 0xa, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
    };

    EXPECT_LE(golden.size(), fileContents->size());
    for (size_t i = 0; i < golden.size(); i++) {
        EXPECT_EQ(golden.at(i), fileContents->at(i));
    }
}
