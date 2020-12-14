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

#include "android/base/files/CompressingStream.h"

#include "android/base/files/StreamSerializing.h"
#include "lz4.h"

#include <errno.h>

namespace android {
namespace base {

CompressingStream::CompressingStream(Stream& output)
    : mOutput(output), mLzStream(LZ4_createStream()) {}

CompressingStream::~CompressingStream() {
    saveBuffer(&mOutput, mBuffer);
    LZ4_freeStream((LZ4_stream_t*)mLzStream);
}

ssize_t CompressingStream::read(void*, size_t) {
    return -EPERM;
}

ssize_t CompressingStream::write(const void* buffer, size_t size) {
    if (!size) {
        return 0;
    }
    const auto outSize = LZ4_compressBound(size);
    auto oldSize = mBuffer.size();
    mBuffer.resize_noinit(mBuffer.size() + outSize);
    const auto outBuffer = mBuffer.data() + oldSize;
    const int written = LZ4_compress_fast_continue((LZ4_stream_t*)mLzStream,
                                                   (const char*)buffer,
                                                   outBuffer, size, outSize, 1);
    if (!written) {
        return -EIO;
    }
    mBuffer.resize(oldSize + written);
    return size;
}

}  // namespace base
}  // namespace android
