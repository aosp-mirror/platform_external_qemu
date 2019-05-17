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

#include "android/base/Pool.h"

#include <vector>

#include <inttypes.h>

#define E(fmt,...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

namespace goldfish_vk {

class VulkanStream::Impl : public android::base::Stream {
public:
    Impl(IOStream* stream)
        : mStream(stream) { unsetHandleMapping(); }

    ~Impl() { }

    void setStream(IOStream* stream) {
        mStream = stream;
    }

    bool valid() { return true; }

    void alloc(void **ptrAddr, size_t bytes) {
        if (!bytes) {
            *ptrAddr = nullptr;
            return;
        }

        *ptrAddr = mPool.alloc(bytes);
    }

    ssize_t write(const void *buffer, size_t size) override {
        return bufferedWrite(buffer, size);
    }

    ssize_t read(void *buffer, size_t size) override {
        commitWrite();
        if (!mStream->readFully(buffer, size)) {
            E("FATAL: Could not read back %zu bytes", size);
            abort();
        }
        return size;
    }

    void flush() {
        commitWrite();
    }

    void clearPool() {
        mPool.freeAll();
    }

    void setHandleMapping(VulkanHandleMapping* mapping) {
        mCurrentHandleMapping = mapping;
    }

    void unsetHandleMapping() {
        mCurrentHandleMapping = &mDefaultHandleMapping;
    }

    VulkanHandleMapping* handleMapping() const {
        return mCurrentHandleMapping;
    }

private:
    size_t oustandingWriteBuffer() const {
        return mWritePos;
    }

    size_t remainingWriteBufferSize() const {
        return mWriteBuffer.size() - mWritePos;
    }

    void commitWrite() {
        if (!valid()) {
            E("FATAL: Tried to commit write to vulkan pipe with invalid pipe!");
            abort();
        }

        int written =
            mStream->writeFully(mWriteBuffer.data(), mWritePos);

        if (written) {
            E("FATAL: Did not write exactly %zu bytes!", mWritePos);
            abort();
        }
        mWritePos = 0;
    }

    ssize_t bufferedWrite(const void *buffer, size_t size) {
        if (size > remainingWriteBufferSize()) {
            mWriteBuffer.resize((mWritePos + size) << 1);
        }
        memcpy(mWriteBuffer.data() + mWritePos, buffer, size);
        mWritePos += size;
        return size;
    }

    android::base::Pool mPool { 8, 4096, 64 };

    size_t mWritePos = 0;
    std::vector<uint8_t> mWriteBuffer;
    IOStream* mStream = nullptr;
    DefaultHandleMapping mDefaultHandleMapping;
    VulkanHandleMapping* mCurrentHandleMapping;
};

VulkanStream::VulkanStream(IOStream *stream) :
    mImpl(new VulkanStream::Impl(stream)) { }

VulkanStream::~VulkanStream() = default;

void VulkanStream::setStream(IOStream* stream) {
    mImpl->setStream(stream);
}

bool VulkanStream::valid() {
    return mImpl->valid();
}

void VulkanStream::alloc(void** ptrAddr, size_t bytes) {
    // Do not free *ptrAddr if bytes == 0, which causes problems
    // with the protocol and overwrites guest user data.
    if (bytes == 0) {
        return;
    }

    mImpl->alloc(ptrAddr, bytes);
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

ssize_t VulkanStream::read(void *buffer, size_t size) {
    return mImpl->read(buffer, size);
}

ssize_t VulkanStream::write(const void *buffer, size_t size) {
    return mImpl->write(buffer, size);
}

void VulkanStream::commitWrite() {
    mImpl->flush();
}

void VulkanStream::clearPool() {
    mImpl->clearPool();
}

void VulkanStream::setHandleMapping(VulkanHandleMapping* mapping) {
    mImpl->setHandleMapping(mapping);
}

void VulkanStream::unsetHandleMapping() {
    mImpl->unsetHandleMapping();
}

VulkanHandleMapping* VulkanStream::handleMapping() const {
    return mImpl->handleMapping();
}

VulkanMemReadingStream::VulkanMemReadingStream(uint8_t* start)
    : mStart(start), VulkanStream(nullptr) {}

VulkanMemReadingStream::~VulkanMemReadingStream() { }

void VulkanMemReadingStream::setBuf(uint8_t* buf) {
    mStart = buf;
    mReadPos = 0;
    resetTrace();
}

ssize_t VulkanMemReadingStream::read(void* buffer, size_t size) {
    memcpy(buffer, mStart + mReadPos, size);
    mReadPos += size;
    return size;
}

ssize_t VulkanMemReadingStream::write(const void* buffer, size_t size) {
    fprintf(stderr,
            "%s: FATAL: VulkanMemReadingStream does not support writing\n",
            __func__);
    abort();
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
