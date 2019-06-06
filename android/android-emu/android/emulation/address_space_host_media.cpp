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

#include "android/emulation/address_space_device.hpp"
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

namespace {

enum class DecoderType : uint8_t {
    Vpx = 0,
    H264 = 1,
};

void* getHostAddress(uint64_t guest_phys_addr) {
    void* ptr = goldfish_address_space_get_vm_operations()->physicalMemoryGetAddr(guest_phys_addr);
    return ptr;
}

};  // namespace

AddressSpaceHostMediaContext::AddressSpaceHostMediaContext(uint64_t phys_addr, bool fromSnapshot) {
    // The memory is allocated in the snapshot load if called from a snapshot load().
    if (!fromSnapshot) {
        mGuestAddr = phys_addr;
        mPhysAddr = allocatePages(phys_addr, kNumPages);
    }
}

void AddressSpaceHostMediaContext::perform(AddressSpaceDevicePingInfo *info) {
    handleMediaRequest(info);
}

AddressSpaceDeviceType AddressSpaceHostMediaContext::getDeviceType() const {
    return AddressSpaceDeviceType::Media;
}

void AddressSpaceHostMediaContext::save(base::Stream* stream) const {
    AS_DEVICE_DPRINT("Saving Host Media snapshot");
    stream->putBe64(mGuestAddr);
    stream->putBe32(mNumActiveDecoders);
    if (mVpxDecoder != nullptr) {
        AS_DEVICE_DPRINT("Saving VpxDecoder snapshot");
        stream->putBe32((uint32_t)DecoderType::Vpx);
        mVpxDecoder->save(stream);
    }
    if (mH264Decoder != nullptr) {
        AS_DEVICE_DPRINT("Saving H264Decoder snapshot");
        stream->putBe32((uint32_t)DecoderType::H264);
        mH264Decoder->save(stream);
    }
}

bool AddressSpaceHostMediaContext::load(base::Stream* stream) {
    AS_DEVICE_DPRINT("Loading Host Media snapshot");
    mGuestAddr = stream->getBe64();
    mPhysAddr = allocatePages(mGuestAddr, kNumPages);

    mNumActiveDecoders = stream->getBe32();
    for (int i = 0; i < mNumActiveDecoders; ++i) {
        DecoderType t = (DecoderType)stream->getBe32();
        switch (t) {
        case DecoderType::Vpx:
            AS_DEVICE_DPRINT("Loading VpxDecoder snapshot");
            mVpxDecoder.reset(new MediaVpxDecoder);
            mVpxDecoder->load(stream);
            break;
        case DecoderType::H264:
            AS_DEVICE_DPRINT("Loading H264Decoder snapshot");
            mH264Decoder.reset(MediaH264Decoder::create());
            mH264Decoder->load(stream);
            break;
        default:
            break;
        }
    }
    return true;
}

// static
void* AddressSpaceHostMediaContext::allocatePages(uint64_t phys_addr, int num_pages) {
    void* hostPhysAddr = android::aligned_buf_alloc(kAlignment, num_pages * kAlignment);
    goldfish_address_space_get_vm_operations()->mapUserBackedRam(phys_addr, hostPhysAddr, num_pages * kAlignment);
    AS_DEVICE_DPRINT("Allocating host memory for media context: guest_addr 0x%11x, 0x%11x",
                     phys_addr, hostPhysAddr);
    return hostPhysAddr;
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
            if (!mVpxDecoder) {
                mVpxDecoder.reset(new MediaVpxDecoder);
                ++mNumActiveDecoders;
            }
            mVpxDecoder->handlePing(codecType,
                                   op,
                                   getHostAddress(info->phys_addr));
            break;
        case MediaCodecType::H264Codec:
            if (!mH264Decoder) {
                mH264Decoder.reset(MediaH264Decoder::create());
                ++mNumActiveDecoders;
            }
            mH264Decoder->handlePing(codecType,
                                     op,
                                     getHostAddress(info->phys_addr));
            break;
    }
}

}  // namespace emulation
}  // namespace android
