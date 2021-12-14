// Copyright (C) 2018 The Android Open Source Project
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
#include "VulkanStream.h"

#include "IOStream.h"

#include "android/base/BumpPool.h"
#include "android/utils/GfxstreamFatalError.h"

#include "emugl/common/feature_control.h"

#include <vector>

#include <inttypes.h>

#define E(fmt,...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

using emugl::emugl_feature_is_enabled;

namespace goldfish_vk {

VulkanStream::VulkanStream(IOStream *stream) : mStream(stream) {
    unsetHandleMapping();

    if (emugl_feature_is_enabled(android::featurecontrol::VulkanNullOptionalStrings)) {
        mFeatureBits |= VULKAN_STREAM_FEATURE_NULL_OPTIONAL_STRINGS_BIT;
    }
    if (emugl_feature_is_enabled(android::featurecontrol::VulkanIgnoredHandles)) {
        mFeatureBits |= VULKAN_STREAM_FEATURE_IGNORED_HANDLES_BIT;
    }
    if (emugl_feature_is_enabled(android::featurecontrol::VulkanShaderFloat16Int8)) {
        mFeatureBits |= VULKAN_STREAM_FEATURE_SHADER_FLOAT16_INT8_BIT;
    }
}

VulkanStream::~VulkanStream() = default;

void VulkanStream::setStream(IOStream* stream) {
    mStream = stream;
}

bool VulkanStream::valid() {
    return true;
}

void VulkanStream::alloc(void** ptrAddr, size_t bytes) {
    if (!bytes) {
        *ptrAddr = nullptr;
        return;
    }

    *ptrAddr = mPool.alloc(bytes);

    if (!*ptrAddr) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) <<
                "alloc failed. Wanted size: " << bytes;
    }
}

void VulkanStream::loadStringInPlace(char** forOutput) {
    size_t len = getBe32();

    alloc((void**)forOutput, len + 1);

    memset(*forOutput, 0x0, len + 1);

    if (len > 0) read(*forOutput, len);
}

void VulkanStream::loadStringArrayInPlace(char*** forOutput) {
    size_t count = getBe32();

    if (!count) {
        *forOutput = nullptr;
        return;
    }

    alloc((void**)forOutput, count * sizeof(char*));

    char **stringsForOutput = *forOutput;

    for (size_t i = 0; i < count; i++) {
        loadStringInPlace(stringsForOutput + i);
    }
}

void VulkanStream::loadStringInPlaceWithStreamPtr(char** forOutput, uint8_t** streamPtr) {
    uint32_t len;
    memcpy(&len, *streamPtr, sizeof(uint32_t));
    *streamPtr += sizeof(uint32_t);
    android::base::Stream::fromBe32((uint8_t*)&len);

    if (len == UINT32_MAX) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) <<
                "VulkanStream can't allocate UINT32_MAX bytes";
    }

    alloc((void**)forOutput, len + 1);

    if (len > 0) {
        memcpy(*forOutput, *streamPtr, len);
        *streamPtr += len;
    }
    (*forOutput)[len] = 0;
}

void VulkanStream::loadStringArrayInPlaceWithStreamPtr(char*** forOutput, uint8_t** streamPtr) {
    uint32_t count;
    memcpy(&count, *streamPtr, sizeof(uint32_t));
    *streamPtr += sizeof(uint32_t);
    android::base::Stream::fromBe32((uint8_t*)&count);

    if (!count) {
        *forOutput = nullptr;
        return;
    }

    alloc((void**)forOutput, count * sizeof(char*));

    char **stringsForOutput = *forOutput;

    for (size_t i = 0; i < count; i++) {
        loadStringInPlaceWithStreamPtr(stringsForOutput + i, streamPtr);
    }
}

ssize_t VulkanStream::read(void *buffer, size_t size) {
    commitWrite();
    if (!mStream->readFully(buffer, size)) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Could not read back " << size << " bytes";
    }
    return size;
}

size_t VulkanStream::remainingWriteBufferSize() const {
    return mWriteBuffer.size() - mWritePos;
}

ssize_t VulkanStream::bufferedWrite(const void *buffer, size_t size) {
    if (size > remainingWriteBufferSize()) {
        mWriteBuffer.resize((mWritePos + size) << 1);
    }
    memcpy(mWriteBuffer.data() + mWritePos, buffer, size);
    mWritePos += size;
    return size;
}

ssize_t VulkanStream::write(const void *buffer, size_t size) {
    return bufferedWrite(buffer, size);
}

void VulkanStream::commitWrite() {
    if (!valid()) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) <<
                            "Tried to commit write to vulkan pipe with invalid pipe!";
    }

    int written =
        mStream->writeFully(mWriteBuffer.data(), mWritePos);

    if (written) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "Did not write exactly " << mWritePos << " bytes!";
    }
    mWritePos = 0;
}

void VulkanStream::clearPool() {
    mPool.freeAll();
}

void VulkanStream::setHandleMapping(VulkanHandleMapping* mapping) {
    mCurrentHandleMapping = mapping;
}

void VulkanStream::unsetHandleMapping() {
    mCurrentHandleMapping = &mDefaultHandleMapping;
}

VulkanHandleMapping* VulkanStream::handleMapping() const {
    return mCurrentHandleMapping;
}

uint32_t VulkanStream::getFeatureBits() const {
    return mFeatureBits;
}

android::base::BumpPool* VulkanStream::pool() {
    return &mPool;
}

VulkanMemReadingStream::VulkanMemReadingStream(uint8_t* start)
    : mStart(start), VulkanStream(nullptr) {}

VulkanMemReadingStream::~VulkanMemReadingStream() { }

void VulkanMemReadingStream::setBuf(uint8_t* buf) {
    mStart = buf;
    mReadPos = 0;
    resetTrace();
}

uint8_t* VulkanMemReadingStream::getBuf() {
    return mStart;
}

void VulkanMemReadingStream::setReadPos(uintptr_t pos) {
    mReadPos = pos;
}

ssize_t VulkanMemReadingStream::read(void* buffer, size_t size) {
    memcpy(buffer, mStart + mReadPos, size);
    mReadPos += size;
    return size;
}

ssize_t VulkanMemReadingStream::write(const void* buffer, size_t size) {
    GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) <<
            "VulkanMemReadingStream does not support writing";
}

uint8_t* VulkanMemReadingStream::beginTrace() {
    resetTrace();
    return mTraceStart;
}

size_t VulkanMemReadingStream::endTrace() {
    uintptr_t current = (uintptr_t)(mStart + mReadPos);
    size_t res = (size_t)(current - (uintptr_t)mTraceStart);
    return res;
}

void VulkanMemReadingStream::resetTrace() {
    mTraceStart = mStart + mReadPos;
}

} // namespace goldfish_vk
