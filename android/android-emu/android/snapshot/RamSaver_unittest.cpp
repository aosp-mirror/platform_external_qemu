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

#include "android/base/testing/TestTempDir.h"
#include "android/base/misc/FileUtils.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

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
        fprintf(stderr, "RamSaverTest::%s\n", __func__);
        mTempDir.reset(new TestTempDir("ramsavertest"));
    }

    void TearDown() override {
        fprintf(stderr, "RamSaverTest::%s\n", __func__);
        mTempDir.reset();
    }

    std::unique_ptr<TestTempDir> mTempDir;
};

// Save 10 pages that are all zero with no compression.
TEST_F(RamSaverTest, Simple) {
    std::string ramSaverTestPath =
        mTempDir->makeSubPath("ram.bin");

    std::vector<uint8_t> testRam(10 * 4096, 0);

    RamBlock block = {
        "ramSaverTestBlock",
        0x0,
        testRam.data(),
        (int64_t)testRam.size(),
        4096,
    };

    RamSaver s(ramSaverTestPath.c_str(), RamSaver::Flags::None, nullptr, true);

    s.registerBlock(block);

    s.savePage(0x0, 0x0000, 0x1000);
    s.savePage(0x0, 0x1000, 0x1000);
    s.savePage(0x0, 0x2000, 0x1000);
    s.savePage(0x0, 0x3000, 0x1000);
    s.savePage(0x0, 0x4000, 0x1000);

    s.join();

    auto fileContents = android::readFileIntoString(ramSaverTestPath);

    std::vector<uint8_t> golden = {
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
        fprintf(stderr, "%s: %zu\n", __func__, i);
        EXPECT_EQ(golden.at(i), fileContents->at(i));
    }
}
