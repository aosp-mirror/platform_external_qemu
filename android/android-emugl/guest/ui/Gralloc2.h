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
#ifndef ANDROID_UI_GRALLOC2_H
#define ANDROID_UI_GRALLOC2_H
#include <string>
#include <cutils/native_handle.h>
#include <utils/StrongPointer.h>
#include <stdint.h>
#include <ui/PixelFormat.h>

namespace android {
struct YCbCrLayout {
    void* y;
    void* cb;
    void* cr;
    uint32_t yStride;
    uint32_t cStride;
    uint32_t chromaStep;
};

namespace hardware {
typedef buffer_handle_t hidl_handle;
} // namespace hardware

namespace Gralloc2 {

using android::PixelFormat;
using android::YCbCrLayout;

namespace IMapper {

struct BufferDescriptorInfo {
    uint32_t width;
    uint32_t height;
    uint32_t layerCount;
    uint32_t format;
    uint32_t usage;
};

struct Rect {
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
};

} // namespace IMapper

typedef IMapper::BufferDescriptorInfo* BufferDescriptor;

enum Error {
    NONE = 0,
    BAD_VALUE = -22,
    NO_RESOURCES = -12,
};

// A wrapper to IMapper
class Mapper {
public:
    static void preload();
    Mapper();
    Error createDescriptor(
            const IMapper::BufferDescriptorInfo& descriptorInfo,
            BufferDescriptor* outDescriptor) const;
    // Import a buffer that is from another HAL, another process, or is
    // cloned.
    //
    // The returned handle must be freed with freeBuffer.
    Error importBuffer(const hardware::hidl_handle& rawHandle,
            buffer_handle_t* outBufferHandle) const;
    void freeBuffer(buffer_handle_t bufferHandle) const;
    Error validateBufferSize(buffer_handle_t bufferHandle,
            const IMapper::BufferDescriptorInfo& descriptorInfo,
            uint32_t stride) const;
    void getTransportSize(buffer_handle_t bufferHandle,
            uint32_t* outNumFds, uint32_t* outNumInts) const;
    // The ownership of acquireFence is always transferred to the callee, even
    // on errors.
    Error lock(buffer_handle_t bufferHandle, uint64_t usage,
            const IMapper::Rect& accessRegion,
            int acquireFence, void** outData) const;
    // The ownership of acquireFence is always transferred to the callee, even
    // on errors.
    Error lock(buffer_handle_t bufferHandle, uint64_t usage,
            const IMapper::Rect& accessRegion,
            int acquireFence, YCbCrLayout* outLayout) const;
    // unlock returns a fence sync object (or -1) and the fence sync object is
    // owned by the caller
    int unlock(buffer_handle_t bufferHandle) const;
private:
    // Determines whether the passed info is compatible with the mapper.
    Error validateBufferDescriptorInfo(
            const IMapper::BufferDescriptorInfo& descriptorInfo) const;
};

// A wrapper to IAllocator
class Allocator {
public:
    // An allocator relies on a mapper, and that mapper must be alive at all
    // time.
    Allocator(const Mapper& mapper);
    std::string dumpDebugInfo() const;
    /*
     * The returned buffers are already imported and must not be imported
     * again.  outBufferHandles must point to a space that can contain at
     * least "count" buffer_handle_t.
     */
    Error allocate(BufferDescriptor descriptor, uint32_t count,
            uint32_t* outStride, buffer_handle_t* outBufferHandles) const;
    Error allocate(BufferDescriptor descriptor,
            uint32_t* outStride, buffer_handle_t* outBufferHandle) const
    {
        return allocate(descriptor, 1, outStride, outBufferHandle);
    }
    Error allocate(const IMapper::BufferDescriptorInfo& descriptorInfo, uint32_t count,
            uint32_t* outStride, buffer_handle_t* outBufferHandles) const
    {
        BufferDescriptor descriptor;
        Error error = mMapper.createDescriptor(descriptorInfo, &descriptor);
        if (error == Error::NONE) {
            error = allocate(descriptor, count, outStride, outBufferHandles);
        }
        return error;
    }
    Error allocate(const IMapper::BufferDescriptorInfo& descriptorInfo,
            uint32_t* outStride, buffer_handle_t* outBufferHandle) const
    {
        return allocate(descriptorInfo, 1, outStride, outBufferHandle);
    }
private:
    const Mapper& mMapper;
};
} // namespace Gralloc2
} // namespace android
#endif // ANDROID_UI_GRALLOC2_H
