// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/InplaceStream.h"

#include "android/base/files/StreamSerializing.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace android {
namespace base {

ssize_t InplaceStream::read(void* buffer, size_t size) {
    const auto sizeToRead = std::min<int>(size, readSize());
    memcpy(buffer, mData + readPos(), sizeToRead);
    mReadPos += sizeToRead;
    return sizeToRead;
}

ssize_t InplaceStream::write(const void* buffer, size_t size) {
    const auto sizeToWrite = std::min<int>(mDataLen - writtenSize(), size);
    memcpy(mData + mWritePos, buffer, sizeToWrite);
    mWritePos += sizeToWrite;
    return sizeToWrite;
}

int InplaceStream::writtenSize() const {
    return mWritePos;
}

int InplaceStream::readPos() const {
    return mReadPos;
}

int InplaceStream::readSize() const {
    return mDataLen - mReadPos;
}

void InplaceStream::save(Stream* stream) const {
    saveBufferRaw(stream, mData, writtenSize());
}

void InplaceStream::load(Stream* stream) {
    loadBufferRaw(stream, mData);
    mReadPos = 0;
}

}  // namespace base
}  // namespace android
