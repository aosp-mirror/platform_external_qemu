/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "ReadBuffer.h"

#include <assert.h>
#include <string.h>

static bool isAligned(const void* ptr, size_t alignment) {
    return ((uintptr_t)ptr & (alignment - 1U)) == 0;
}

ReadBuffer::ReadBuffer(IOStream* stream) : mStream(stream) {
    mBuffers.resize(20);
    mWriteIndex = mSavedReadIndex = mReadIndex = mBuffers.begin();
    mSavedReadBufferOffset = mReadBufferOffset = mReadIndex->begin();
}

const char* ReadBuffer::readAlignedChunk(int size, size_t alignment) {
    const auto buf = mReadBufferOffset;
    const auto bufSize = mReadIndex->end() - buf;
    if (isAligned(buf, alignment) && bufSize >= size) {
        advanceReadBy(size);
        return buf;
    }

    // allocate new aligned array and copy the data over into there
    if (mFreeAlignedArrayIndex == (int)mAlignedArrays.size()) {
        mAlignedArrays.emplace_back();
        mAlignedArrays[mAlignedArrays.size() - 1].resize_noinit(
                std::max((int)size, 16));
    }
    auto& array = mAlignedArrays[mFreeAlignedArrayIndex++];
    array.resize_noinit(size);
    auto ptr = array.data();
    if (size <= bufSize) {
        memcpy(ptr, buf, size);
        advanceReadBy(size);
    } else {
        memcpy(ptr, buf, bufSize);
        ptr += bufSize;
        advanceReadIndex();
        for (;;) {
            const auto& buffer = *mReadIndex;
            const auto curSize =
                    std::min(array.end() - ptr, (long)buffer.size());
            memcpy(ptr, buffer.data(), curSize);
            ptr += curSize;
            if (ptr == array.end()) {
                advanceReadBy(curSize);
                break;
            } else {
                advanceReadIndex();
            }
        }
    }

    return array.data();
}

void ReadBuffer::consume(int actuallyRead, int expectedToRead) {
    assert(actuallyRead <= expectedToRead);
    assert(expectedToRead <= mTotalByteSize);
    if (actuallyRead < expectedToRead) {
        // We've read less than expected, skip the extra size.
        int extraSize = expectedToRead - actuallyRead;
        const auto buf = mReadBufferOffset;
        const auto remainingSize = mReadIndex->end() - buf;
        if (remainingSize >= extraSize) {
            advanceReadBy(extraSize);
        } else {
            advanceReadIndex();
            extraSize -= remainingSize;
            for (;;) {
                const auto& buffer = *mReadIndex;
                if ((int)buffer.size() >= extraSize) {
                    advanceReadBy(extraSize);
                    break;
                } else {
                    extraSize -= buffer.size();
                    advanceReadIndex();
                }
            }
        }
    }
    mFreeAlignedArrayIndex = 0;
    mTotalByteSize -= expectedToRead;
    mSavedReadIndex = mReadIndex;
    mSavedReadBufferOffset = mReadBufferOffset;
}

void ReadBuffer::rewind() {
    assert(mFreeAlignedArrayIndex == 0);
    assert(mReadIndex == mSavedReadIndex);
    mReadBufferOffset = mSavedReadBufferOffset;
}

void ReadBuffer::advanceReadIndex() {
    assert(mReadIndex != mWriteIndex);
    if (++mReadIndex == mBuffers.end()) {
        mReadIndex = mBuffers.begin();
    }
    mReadBufferOffset = mReadIndex->data();
}

void ReadBuffer::advanceReadBy(int size) {
    mReadBufferOffset += size;
    if (mReadBufferOffset == mReadIndex->end()) {
        advanceReadIndex();
    }
}

void ReadBuffer::growBuffer() {
    // Recalculate all pointers to indexes, as pointers will be
    // invalidated on the following moves.
    const auto readBufferOffset = mReadBufferOffset - mReadIndex->begin();
    const auto readIndex = mReadIndex - mBuffers.begin();
    const auto writeIndex = mWriteIndex - mBuffers.begin();
    const auto savedReadIndex = mSavedReadIndex - mBuffers.begin();
    const auto savedReadBufferOffset =
            mSavedReadBufferOffset - mSavedReadIndex->begin();

    Buffers newBuffers;
    newBuffers.reserve(2 * mBuffers.size());

    // Move ready to read buffers, starting from the saved read index
    int movedSize = -savedReadBufferOffset;
    auto srcBuffer = mSavedReadIndex;
    while (movedSize < mTotalByteSize) {
        movedSize += srcBuffer->size();
        newBuffers.emplace_back(std::move(*srcBuffer));
        if (++srcBuffer == mBuffers.end()) {
            srcBuffer = mBuffers.begin();
        }
    }
    // Now add the missing empty elements.
    newBuffers.resize(2 * mBuffers.size());

    // And correct the indexes. We've moved everything by
    // |savedReadIndex| items to the left.
    auto newReadIndex = readIndex - savedReadIndex;
    if (newReadIndex < 0) {
        newReadIndex += mBuffers.size();
    }
    auto newWriteIndex = writeIndex - savedReadIndex;
    if (newWriteIndex < 0) {
        newWriteIndex += mBuffers.size();
    }

    mBuffers = std::move(newBuffers);

    // Now we're good to calculate our pointers back from the indexes.
    mReadIndex = mBuffers.begin() + newReadIndex;
    mReadBufferOffset = mReadIndex->begin() + readBufferOffset;
    mWriteIndex = mBuffers.begin() + newWriteIndex;
    mSavedReadIndex = mBuffers.begin();
    mSavedReadBufferOffset = mSavedReadIndex->begin() + savedReadBufferOffset;
}

bool ReadBuffer::prefetchImpl(int sizeToRead) {
    assert(sizeToRead > 0);
    while (sizeToRead > 0) {
        // mTotalByteSize == 0 -> we've got no data, so don't care
        // about overwriting it.
        if (mTotalByteSize != 0 && mWriteIndex == mSavedReadIndex) {
            // Oops, we've reached the previous saved state - resize the array.
            growBuffer();
        }

        if (mReadIndex == mWriteIndex) {
            assert(mSavedReadIndex == mReadIndex);
            assert(mReadBufferOffset == mReadIndex->begin());
            assert(mSavedReadBufferOffset == mSavedReadIndex->begin());
        }

        if (!mStream->read(&*mWriteIndex)) {
            // Well, what else can we do here?
            return false;
        }

        if (mReadIndex == mWriteIndex) {
            mSavedReadBufferOffset = mSavedReadIndex->begin();
            mReadBufferOffset = mReadIndex->begin();
        }

        const auto readSize = mWriteIndex->size();
        mTotalByteSize += readSize;

        sizeToRead -= readSize;
        if (++mWriteIndex == mBuffers.end()) {
            mWriteIndex = mBuffers.begin();
        }
    }

    return true;
}
