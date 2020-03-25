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
#include "android/emulation/address_space_device.h"
#include "android/emulation/GoldfishMediaDefs.h"
#include "android/emulation/MediaVpxDecoder.h"
#include "android/emulation/MediaH264Decoder.h"

#include <unordered_map>

namespace android {
namespace emulation {

class AddressSpaceHostMediaContext : public AddressSpaceDeviceContext {
public:
    AddressSpaceHostMediaContext(uint64_t phys_addr, const address_space_device_control_ops* ops, bool fromSnapshot);
    virtual ~AddressSpaceHostMediaContext();
    void perform(AddressSpaceDevicePingInfo *info) override;

    AddressSpaceDeviceType getDeviceType() const override;
    void save(base::Stream* stream) const override;
    bool load(base::Stream* stream) override;

private:
    void allocatePages(uint64_t phys_addr, int num_pages);
    void deallocatePages(uint64_t phys_addr, int num_pages);
    void handleMediaRequest(AddressSpaceDevicePingInfo *info);
    static MediaCodecType getMediaCodecType(uint64_t metadata);
    static MediaOperation getMediaOperation(uint64_t metadata);

    static constexpr int kAlignment = 4096;
    static constexpr int kNumPages = 1 + 4096 * 2; // 32M + 4k
    bool isMemoryAllocated = false;
    std::unique_ptr<MediaVpxDecoder> mVpxDecoder;
    std::unique_ptr<MediaH264Decoder> mH264Decoder;
    void* mHostBuffer = nullptr;
    const address_space_device_control_ops* mControlOps = 0;
    uint64_t mGuestAddr = 0;
};

}  // namespace emulation
}  // namespace android
