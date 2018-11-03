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
    Impl(IOStream* stream) : mStream(stream) { }

    ~Impl() { }

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
};

VulkanStream::VulkanStream(IOStream *stream) :
    mImpl(new VulkanStream::Impl(stream)) { }

VulkanStream::~VulkanStream() = default;

bool VulkanStream::valid() {
    return mImpl->valid();
}

void VulkanStream::alloc(void** ptrAddr, size_t bytes) {
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

} // namespace goldfish_vk