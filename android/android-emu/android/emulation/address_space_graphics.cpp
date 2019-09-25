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

#include "android/emulation/address_space_device.hpp"
#include "android/emulation/control/vm_operations.h"
#include "android/base/AlignedBuf.h"
#include "android/crashreport/crash-handler.h"

namespace android {
namespace emulation {

class GraphicsBackingMemory {
public:
    GraphicsBackingMemory() {
    }
private:

};

AddressSpaceGraphicsContext::AddressSpaceGraphicsContext(
    const address_space_device_control_ops *ops)
  : m_ops(ops) {
}

AddressSpaceGraphicsContext::~AddressSpaceGraphicsContext() {
}

void AddressSpaceGraphicsContext::perform(AddressSpaceDevicePingInfo *info) {
    uint64_t result;

    switch (static_cast<GraphicsCommand>(info->metadata)) {
    case GraphicsCommand::AllocOrGetOffset:
    case GraphicsCommand::NotifyAvailable:
    default:
        result = -1;
        break;
    }

    info->metadata = result;
}

AddressSpaceDeviceType AddressSpaceGraphicsContext::getDeviceType() const {
    return AddressSpaceDeviceType::Graphics;
}

}  // namespace emulation
}  // namespace android
