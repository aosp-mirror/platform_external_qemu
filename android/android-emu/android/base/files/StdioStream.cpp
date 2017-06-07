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

#include "android/base/files/StdioStream.h"

#include <assert.h>
#include <errno.h>

namespace android {
namespace base {

StdioStream::StdioStream(FILE* file, Ownership ownership) :
    mFile(file), mOwnership(ownership) {}

StdioStream::StdioStream(StdioStream&& other)
    : mFile(other.mFile), mOwnership(other.mOwnership) {
    other.mFile = nullptr;
}

StdioStream& StdioStream::operator=(StdioStream&& other) {
    assert(this != &other);
    close();
    mFile = other.mFile;
    mOwnership = other.mOwnership;
    other.mFile = nullptr;
    return *this;
}

StdioStream::~StdioStream() {
    close();
}

ssize_t StdioStream::read(void* buffer, size_t size) {
    size_t res = ::fread(buffer, 1, size, mFile);
    if (res < size) {
        if (!::feof(mFile)) {
            errno = ::ferror(mFile);
        }
    }
    return static_cast<ssize_t>(res);
}

ssize_t StdioStream::write(const void* buffer, size_t size) {
    size_t res = ::fwrite(buffer, 1, size, mFile);
    if (res < size) {
        if (!::feof(mFile)) {
            errno = ::ferror(mFile);
        }
    }
    return static_cast<ssize_t>(res);
}

void StdioStream::close() {
    if (mOwnership == kOwner && mFile) {
        ::fclose(mFile);
        mFile = nullptr;
    }
}

}  // namespace base
}  // namespace android
