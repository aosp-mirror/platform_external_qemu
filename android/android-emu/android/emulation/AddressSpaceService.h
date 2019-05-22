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
#pragma once

#include <memory>
#include "android/base/files/Stream.h"

namespace android {
namespace emulation {

// Protocol:
// Guest open()'s the device, kernel gets internal handle
// first ioctl is all 0 fields except for metadata,
// which establishes what kind of thing we want to run,
// enumerated here.

// From there, it's all up to the individual device types.

enum AddressSpaceDeviceType {
    // Covers renderControl, opengles, and vulkan
    Graphics = 0,
    // (audio/video codec devices)
    Media = 1,
    Sensors = 2,
    Power = 3,
    // TODO: All other services currently using goldfish pipe
    GenericPipe = 4,
    HostMemoryAllocator = 5,
};

struct AddressSpaceDevicePingInfo {
    uint64_t phys_addr;
    uint64_t size;
    uint64_t metadata;
    uint64_t wait_phys_addr;
    uint32_t wait_flags;
    uint32_t direction;
};

class AddressSpaceDeviceContext {
public:
    virtual ~AddressSpaceDeviceContext() {}
    virtual void perform(AddressSpaceDevicePingInfo *info) = 0;
    virtual AddressSpaceDeviceType getDeviceType() const = 0;
    virtual void save(base::Stream* stream) const = 0;
    virtual bool load(base::Stream* stream) = 0;
};

struct AddressSpaceContextDescription {
    AddressSpaceDevicePingInfo* pingInfo = nullptr;
    uint64_t pingInfoGpa = 0;  // for snapshots
    std::unique_ptr<AddressSpaceDeviceContext> device_context;
};

} // namespace emulation
} // namespace android
