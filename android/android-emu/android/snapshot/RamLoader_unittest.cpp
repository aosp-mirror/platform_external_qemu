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

#include "android/snapshot/RamLoader.h"

#include "android/base/AlignedBuf.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/testing/TestTempDir.h"

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

static constexpr int kPageSize = 4096;
using TestRam = AlignedBuf<uint8_t, kPageSize>;

class RamLoaderTest : public ::testing::Test {
protected:
    void SetUp() override { mTempDir.reset(new TestTempDir("ramloadertest")); }

    void TearDown() override { mTempDir.reset(); }

    void loadRamFromTestData(StringView filename,
                 uint8_t* buffer,
                 size_t size) {

        auto ram =
            fopen(
                PathUtils::join(
                    System::get()->getProgramDirectory(),
                    "testdata",
                    filename).c_str(),
                "rb");

        EXPECT_TRUE(ram);

        RamLoader::RamBlockStructure emptyRamBlockStructure = {};

        // Disallow on-demand load for now.
        RamLoader ramLoader(StdioStream(ram, StdioStream::kOwner),
                            RamLoader::Flags::None,
                            emptyRamBlockStructure);

        const RamBlock block = {
            // block name needs to match our test data.
            "ramSaverTestBlock", 0x0, buffer, (int64_t)size, kPageSize,
        };

        ramLoader.registerBlock(block);

        ramLoader.start(false);
        ramLoader.join();
    }

    std::unique_ptr<TestTempDir> mTempDir;
};

// Load prebuilt random RAM file.
TEST_F(RamLoaderTest, Simple) {

    TestRam testRam(100 * kPageSize);

    loadRamFromTestData("random-ram-100.bin",
                        testRam.data(), testRam.size());
}
