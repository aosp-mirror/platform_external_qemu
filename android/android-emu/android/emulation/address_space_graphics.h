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

#include "android/emulation/AddressSpaceService.h"

#include "android/base/ring_buffer.h"
#include "android/emulation/address_space_device.h"

#include <unordered_map>

namespace android {
namespace emulation {

class AddressSpaceGraphicsContext : public AddressSpaceDeviceContext {
public:
    enum GraphicsCommand {
        AllocOrGetOffset = 0,
        GetSize = 1,
        GuestInitializedRings = 2,
        NotifyAvailable = 3,
        Echo = 4,
    };

    static void init(const address_space_device_control_ops *ops);
    static void clear();

    AddressSpaceGraphicsContext();
    ~AddressSpaceGraphicsContext();

    void perform(AddressSpaceDevicePingInfo *info) override;

    AddressSpaceDeviceType getDeviceType() const override;
    void save(base::Stream* stream) const override;
    bool load(base::Stream* stream) override;

    struct Ring {
        ring_buffer* ring;
        ring_buffer_view view;
    };

private:
    char* mBuffer = 0;

    Ring mToHost;
    Ring mFromHost;

    uint32_t mRingVersion;
};

}  // namespace emulation
}  // namespace android
