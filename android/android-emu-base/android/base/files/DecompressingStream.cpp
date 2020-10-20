// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/DecompressingStream.h"

#include "android/base/files/StreamSerializing.h"
#include "lz4.h"

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

#include <errno.h>
#include <cassert>

namespace android {
namespace base {

DecompressingStream::DecompressingStream(Stream& input)
    : mLzStream(LZ4_createStreamDecode()) {
    loadBuffer(&input, &mBuffer);
}

DecompressingStream::~DecompressingStream() {
    LZ4_freeStreamDecode((LZ4_streamDecode_t*)mLzStream);
}

ssize_t DecompressingStream::read(void* buffer, size_t size) {
    assert(mBufferPos < mBuffer.size() ||
           (mBufferPos == mBuffer.size() && size == 0));
    if (!size) {
        return 0;
    }
    const int read = LZ4_decompress_fast_continue(
            (LZ4_streamDecode_t*)mLzStream, mBuffer.data() + mBufferPos,
            (char*)buffer, size);
    if (!read) {
        return -EIO;
    }
    mBufferPos += read;
    assert(mBufferPos <= mBuffer.size());
    return size;
}

ssize_t DecompressingStream::write(const void*, size_t) {
    return -EPERM;
}

}  // namespace base
}  // namespace android
