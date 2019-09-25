// Copyright 2019 The Android Open Source Project
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

#include "android/emulation/address_space_graphics.h"

#include "android/emulation/hostdevices/HostAddressSpace.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/address_space_device.hpp"

#include <gtest/gtest.h>

namespace android {
namespace emulation {
namespace asg {

class AddressSpaceGraphicsTest : public ::testing::Test {
protected:

    static void SetUpTestCase() {
        goldfish_address_space_set_vm_operations(gQAndroidVmOperations);
    }

    static void TearDownTestCase() { }

    void SetUp() override {
        mDevice = HostAddressSpaceDevice::get();
        AddressSpaceGraphicsContext::init(get_address_space_device_control_ops());
    }

    void TearDown() override {
        AddressSpaceGraphicsContext::clear();
        mDevice->clear();
    }

    HostAddressSpaceDevice* mDevice = nullptr;
};

TEST_F(AddressSpaceGraphicsTest, Basic) { }

} // namespace asg
} // namespace emulation
} // namespace android
