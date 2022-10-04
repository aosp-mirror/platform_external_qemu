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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/files/Stream.h"

namespace android {
namespace base {

// An implementation of the Stream interface for a given char*.
class InplaceStream : public Stream {
public:
    using Buffer = char*;

    InplaceStream(Buffer buf, uint32_t buflen) :
        mData(buf), mDataLen(buflen) { }

    int writtenSize() const;
    int readPos() const;
    int readSize() const;

    const char* currentRead() const;
    char* currentWrite() const;
    ssize_t advanceRead(size_t size);
    ssize_t advanceWrite(size_t size);

    // Stream interface implementation.
    ssize_t read(void* buffer, size_t size) override;
    ssize_t write(const void* buffer, size_t size) override;

    // A way to put/get strings/buffers in-place as well.
    // Returns nullptr if the size of the resulting string
    // is <= 0 (but with a null terminator, that just means
    // something went wrong, so should never occur).
    // Please make sure input |str| is actually
    // null-terminated.
    void putStringNullTerminated(const char* str);
    const char* getStringNullTerminated();

    // Snapshot support.
    void save(Stream* stream) const;
    void load(Stream* stream);

private:
    DISALLOW_COPY_AND_ASSIGN(InplaceStream);

    Buffer mData = nullptr;
    int mDataLen = 0;
    int mReadPos = 0;
    int mWritePos = 0;
};

}  // namespace base
}  // namespace android
