// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/emulation/hostdevices/HostAddressSpace.h"

#include <gtest/gtest.h>

namespace android {

class HostAddressSpaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        mDevice = HostAddressSpaceDevice::get();
    }

    void TearDown() override {
        // mDevice is singleton, no need to tear down
    }
    HostAddressSpaceDevice* mDevice = nullptr;
};

// Tests the constructor.
TEST_F(HostAddressSpaceTest, Basic) { }

// Tests that basic allocations are page aligned.
TEST_F(HostAddressSpaceTest, BasicAlloc) {
    // Alloc something first to throw it off.
    uint64_t off = mDevice->alloc(1);
    uint64_t off2 = mDevice->alloc(1);
    (void)off;
    EXPECT_EQ(0, off2 & 0xfff);
}

// Tests generation of handles.
TEST_F(HostAddressSpaceTest, GenHandle) {
}

} // namespace android
