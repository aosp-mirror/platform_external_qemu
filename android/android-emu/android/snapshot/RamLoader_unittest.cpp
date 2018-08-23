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
#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/system/System.h"
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

class RamLoaderTest : public ::testing::Test {
protected:
    void SetUp() override { mTempDir.reset(new TestTempDir("ramloadertest")); }

    void TearDown() override { mTempDir.reset(); }

    std::unique_ptr<TestTempDir> mTempDir;
};

// Load prebuilt random RAM file.
// Disabled until new testdata prebuilt is merged
// TEST_F(RamLoaderTest, Simple) {
//     auto path = PathUtils::join(System::get()->getProgramDirectory(),
//                                 "testdata", "random-ram-100.bin");
//
//     TestRamBuffer testRam(100 * kTestingPageSize);
//
//     loadRamSingleBlock({"ramSaverTestBlock", 0x0, testRam.data(),
//                         (int64_t)testRam.size(), kTestingPageSize,
//                         0, nullptr, false, false},
//                        path);
// }

}  // namespace snapshot
}  // namespace android
