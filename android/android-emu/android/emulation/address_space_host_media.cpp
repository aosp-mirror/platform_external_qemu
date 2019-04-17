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

#include "android/emulation/address_space_host_media.h"
#include "android/emulation/control/vm_operations.h"
#include "android/base/AlignedBuf.h"

#define AS_DEVICE_DEBUG 1

#if AS_DEVICE_DEBUG
#define AS_DEVICE_DPRINT(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define AS_DEVICE_DPRINT(fmt,...)
#endif

namespace android {
namespace emulation {

AddressSpaceHostMediaContext::AddressSpaceHostMediaContext(uint64_t phys_addr) {
    allocatePages(phys_addr, kNumPages);
}

void AddressSpaceHostMediaContext::perform(AddressSpaceDevicePingInfo *info) {
    handleMediaRequest(info);
}

void AddressSpaceHostMediaContext::allocatePages(uint64_t phys_addr, int num_pages) {
    void* myptr = android::aligned_buf_alloc(kAlignment, num_pages * 4096);
    gQAndroidVmOperations->mapUserBackedRam(phys_addr, myptr, num_pages * 4096);
    AS_DEVICE_DPRINT("Allocating host memory for media context: guest_addr 0x%11x, 0x%11x",
                     phys_addr, myptr);
}

// static
MediaCodecType AddressSpaceHostMediaContext::getMediaCodecType(uint64_t metadata) {
    // Metadata has the following structure:
    // - Upper 8 bits has the codec type (MediaCodecType)
    // - Lower 56 bits has metadata specifically for that codec
    //
    // We need to hand the data off to the right codec depending on which
    // codec type we get.
    uint8_t ret = (uint8_t)(metadata >> (64 - 8));
    return ret > static_cast<uint8_t>(MediaCodecType::Max) ?
            MediaCodecType::Max : (MediaCodecType)ret;
}

// static
MediaOperation AddressSpaceHostMediaContext::getMediaOperation(uint64_t metadata) {
    // Metadata has the following structure:
    // - Upper 8 bits has the codec type (MediaCodecType)
    // - Lower 56 bits has metadata specifically for that codec
    //
    // We need to hand the data off to the right codec depending on which
    // codec type we get.
    uint8_t ret = (uint8_t)(metadata & 0xFF);
    return ret > static_cast<uint8_t>(MediaOperation::Max) ?
            MediaOperation::Max : (MediaOperation)ret;
}

void AddressSpaceHostMediaContext::handleMediaRequest(AddressSpaceDevicePingInfo *info) {
    auto codecType = getMediaCodecType(info->metadata);
    auto op = getMediaOperation(info->metadata);

    AS_DEVICE_DPRINT("Got media request (type=%u, op=%u)", static_cast<uint8_t>(codecType),
                     static_cast<uint8_t>(op));

    switch (codecType) {
        case MediaCodecType::VP8Codec:
        case MediaCodecType::VP9Codec:
            mVpxDecoder.handlePing(codecType,
                                   op,
                                   AddressSpaceHostMediaContext::getHostAddress(info->phys_addr));
            break;
        case MediaCodecType::H264Codec:
            AS_DEVICE_DPRINT("H264Codec not implemented");
            break;
    }
}

// static
void* AddressSpaceHostMediaContext::getHostAddress(uint64_t guest_phys_addr) {
    void* ptr = gQAndroidVmOperations->physicalMemoryGetAddr(guest_phys_addr);
    return ptr;
}

}  // namespace emulation
}  // namespace android
