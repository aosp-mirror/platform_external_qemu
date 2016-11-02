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
#pragma once
#include "IOStream.h"

#include "android/base/containers/SmallVector.h"
#include <vector>
#include <utility>

#include <assert.h>

class ReadBuffer {
public:
    ReadBuffer(IOStream* stream);

    bool prefetch(int wantedCapacity) {
        if (wantedCapacity <= mTotalByteSize) {
            return true;
        }
        return prefetchImpl(wantedCapacity - mTotalByteSize);
    }

    // read small chunk of data (up to 8 bytes)
    const char* read(int size) {
        assert(mReadIndex->end() - mReadBufferOffset >= size);
        const auto res = mReadBufferOffset;
        // manually inline advanceReadBy(size) here as it's a very hot place.
        if (__builtin_expect(res + size == mReadIndex->end(), false)) {
            advanceReadIndex();
        } else {
            mReadBufferOffset = res + size;
        }
        return res;
    }

    const char* readAlignedChunk(int size, size_t alignment = 8);

    void consume(int actuallyRead, int expectedToRead);
    void rewind();

private:
    void advanceReadIndex();
    void advanceReadBy(int size);

    void growBuffer();
    bool prefetchImpl(int sizeToRead);

private:
    using Buffers = std::vector<emugl::ChannelBuffer>;

    const char* mReadBufferOffset;
    Buffers::iterator mReadIndex;
    int mTotalByteSize = 0;

    Buffers mBuffers;

    // Current read and write positions into the |mBuffers|
    Buffers::iterator mWriteIndex;

    // The last good read position. rewind() reverts back to it.
    Buffers::iterator mSavedReadIndex;
    const char* mSavedReadBufferOffset;

    // A collection of aligned storage arrays. These are used for input buffers.
    // Note: small size of 1 means "just allocate it dynamically". We need small
    //  vectors here not for their inplace buffer but for resize_noinit(), and
    //  we actually really want to have them dynamically allocated, so they
    //  don't invalidate pointers when |mAlignedArrays| has to grow.
    android::base::SmallFixedVector<android::base::SmallFixedVector<char, 1>, 4>
            mAlignedArrays;
    int mFreeAlignedArrayIndex = 0;

    IOStream* mStream;
};
