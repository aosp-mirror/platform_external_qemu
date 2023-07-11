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

#include "aemu/base/files/MemStream.h"

#include "aemu/base/files/StreamSerializing.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace android {
namespace base {

MemStream::MemStream(int reserveSize) {
    mData.reserve(reserveSize);
}

MemStream::MemStream(Buffer&& data) : mData(std::move(data)) {}

ssize_t MemStream::read(void* buffer, size_t size) {
    if (!buffer) {
        return 0;
    }
    const auto sizeToRead = std::min<int>(size, readSize());
    memcpy(buffer, mData.data() + mReadPos, sizeToRead);
    mReadPos += sizeToRead;
    return sizeToRead;
}

ssize_t MemStream::write(const void* buffer, size_t size) {
    if (!buffer) {
        return 0;
    }
    mData.insert(mData.end(), (const char*)buffer, (const char*)buffer + size);
    return size;
}

int MemStream::writtenSize() const {
    return (int)mData.size();
}

int MemStream::readPos() const {
    return mReadPos;
}

int MemStream::readSize() const {
    return mData.size() - mReadPos;
}

void MemStream::save(Stream* stream) const {
    saveBuffer(stream, mData);
}

void MemStream::load(Stream* stream) {
    loadBuffer(stream, &mData);
    mReadPos = 0;
}

void MemStream::rewind() {
    mReadPos = 0;
}

}  // namespace base
}  // namespace android
