/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "Gralloc2"

#include <ui/Gralloc2.h>

#include <cutils/native_handle.h>
#include <inttypes.h>
#include <log/log.h>
#include <system/graphics.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"
#include <sync/sync.h>
#pragma clang diagnostic pop

#include <grallocusage/GrallocUsageConversion.h>
#include <cstring>
#include "GrallocDispatch.h"

namespace android {
namespace Gralloc2 {
namespace {
static constexpr Error kTransactionError = Error::NO_RESOURCES;

// uint64_t getValid10UsageBits() {
//     static const uint64_t valid10UsageBits = []() -> uint64_t {
//         using hardware::graphics::common::V1_0::BufferUsage;
//         uint64_t bits = 0;
//         for (const auto bit : hardware::hidl_enum_range<BufferUsage>()) {
//             bits = bits | bit;
//         }
//         // TODO(b/72323293, b/72703005): Remove these additional bits
//         bits = bits | (1 << 10) | (1 << 13);
//         return bits;
//     }();
//     return valid10UsageBits;
// }

// uint64_t getValid11UsageBits() {
//     static const uint64_t valid11UsageBits = []() -> uint64_t {
//         using hardware::graphics::common::V1_1::BufferUsage;
//         uint64_t bits = 0;
//         for (const auto bit : hardware::hidl_enum_range<BufferUsage>()) {
//             bits = bits | bit;
//         }
//         return bits;
//     }();
//     return valid11UsageBits;
// }

}  // anonymous namespace

void Mapper::preload() {
}

Mapper::Mapper()
{
    // mMapper = hardware::graphics::mapper::V2_0::IMapper::getService();
    // if (mMapper == nullptr) {
    //     LOG_ALWAYS_FATAL("gralloc-mapper is missing");
    // }
    // if (mMapper->isRemote()) {
    //     LOG_ALWAYS_FATAL("gralloc-mapper must be in passthrough mode");
    // }
    // // IMapper 2.1 is optional
    // mMapperV2_1 = IMapper::castFrom(mMapper);
}

Gralloc2::Error Mapper::validateBufferDescriptorInfo(
        const IMapper::BufferDescriptorInfo& descriptorInfo) const {
    return Error::NONE;

    // uint64_t validUsageBits = getValid10UsageBits();
    // if (mMapperV2_1 != nullptr) {
    //     validUsageBits = validUsageBits | getValid11UsageBits();
    // }
    // if (descriptorInfo.usage & ~validUsageBits) {
    //     ALOGE("buffer descriptor contains invalid usage bits 0x%" PRIx64,
    //           descriptorInfo.usage & ~validUsageBits);
    //     return Error::BAD_VALUE;
    // }
    // return Error::NONE;
}
Error Mapper::createDescriptor(
        const IMapper::BufferDescriptorInfo& descriptorInfo,
        BufferDescriptor* outDescriptor) const
{
    Error error = validateBufferDescriptorInfo(descriptorInfo);
    if (error != Error::NONE) {
        return error;
    }
    *outDescriptor = new IMapper::BufferDescriptorInfo;
    memcpy(*outDescriptor, &descriptorInfo, sizeof(descriptorInfo));
    return error;
}

Error Mapper::importBuffer(const hardware::hidl_handle& rawHandle,
        buffer_handle_t* outBufferHandle) const
{
    if (!rawHandle) return kTransactionError;
    *outBufferHandle = rawHandle;
    return Error::NONE;
}

void Mapper::freeBuffer(buffer_handle_t bufferHandle) const
{
    auto gralloc = get_global_gralloc_module();
    if (!gralloc) {
        fprintf(stderr, "Mapper::%s: no gralloc module loaded!\n", __func__);
        return;
    }

    gralloc->free(bufferHandle);
}

Error Mapper::validateBufferSize(buffer_handle_t bufferHandle,
        const IMapper::BufferDescriptorInfo& descriptorInfo,
        uint32_t stride) const
{
    return Error::NONE;
}

void Mapper::getTransportSize(buffer_handle_t bufferHandle,
        uint32_t* outNumFds, uint32_t* outNumInts) const
{
    *outNumFds = 1;
    *outNumInts = 1;
}

Error Mapper::lock(buffer_handle_t bufferHandle, uint64_t usage,
        const IMapper::Rect& accessRegion,
        int acquireFence, void** outData) const
{
    (void)acquireFence;

    auto gralloc = get_global_gralloc_module();
    if (!gralloc) {
        fprintf(stderr, "Mapper::%s: no gralloc module loaded!\n", __func__);
        return kTransactionError;
    }

    int32_t usage0 = android_convertGralloc1To0Usage(0, usage);

    int res = gralloc->lock(
            bufferHandle, usage0,
            accessRegion.left, accessRegion.top,
            accessRegion.width, accessRegion.height,
            outData);

    if (res) {
        fprintf(stderr, "Mapper::%s: got error from gralloc lock: %d\n", __func__, res);
        return Error::BAD_VALUE;
    }

    return Error::NONE;
}

Error Mapper::lock(buffer_handle_t bufferHandle, uint64_t usage,
        const IMapper::Rect& accessRegion,
        int acquireFence, YCbCrLayout* outLayout) const {

    (void)acquireFence;

    auto gralloc = get_global_gralloc_module();
    if (!gralloc) {
        fprintf(stderr, "Mapper::%s: no gralloc module loaded!\n", __func__);
        return kTransactionError;
    }

    int32_t usage0 = android_convertGralloc1To0Usage(0, usage);
    android_ycbcr aycbcr;
    int res = gralloc->lock_ycbcr(
            bufferHandle, usage0,
            accessRegion.left, accessRegion.top,
            accessRegion.width, accessRegion.height,
            &aycbcr);

    if (res) {
        fprintf(stderr, "Mapper::%s: got error from gralloc lock_ycbcr: %d\n", __func__, res);
        return Error::BAD_VALUE;
    }

    outLayout->y = aycbcr.y;
    outLayout->cb = aycbcr.cb;
    outLayout->cr = aycbcr.cr;
    outLayout->yStride = aycbcr.ystride;
    outLayout->cStride = aycbcr.cstride;
    outLayout->chromaStep = aycbcr.chroma_step;

    return Error::NONE;
}

int Mapper::unlock(buffer_handle_t bufferHandle) const
{
    auto gralloc = get_global_gralloc_module();

    if (!gralloc) {
        fprintf(stderr, "Mapper::%s: no gralloc module loaded!\n", __func__);
        return kTransactionError;
    }

    int releaseFence = -1;

    int res = gralloc->unlock(bufferHandle);

    if (res) {
        fprintf(stderr, "Mapper::%s: got error from gralloc unlock: %d\n", __func__, res);
        return releaseFence;
    }

    return releaseFence;
}

Allocator::Allocator(const Mapper& mapper)
    : mMapper(mapper)
{ }

std::string Allocator::dumpDebugInfo() const
{
    return "PlaceholderDebugInfo";
}

Error Allocator::allocate(BufferDescriptor descriptor, uint32_t count,
        uint32_t* outStride, buffer_handle_t* outBufferHandles) const
{

    auto gralloc = get_global_gralloc_module();

    if (!gralloc) {
        fprintf(stderr, "Mapper::%s: no gralloc module loaded!\n", __func__);
        return kTransactionError;
    }

    const auto& info = *(IMapper::BufferDescriptorInfo*)(descriptor);

    for (uint32_t i = 0; i < count; ++i) {
        gralloc->alloc(
            info.width, info.height,
            info.format, info.usage,
            outBufferHandles + i, (int32_t*)outStride + i);
    }

    return Error::NONE;
}

} // namespace Gralloc2
} // namespace android

