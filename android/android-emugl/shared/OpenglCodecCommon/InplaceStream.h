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

#include <string>
#include <vector>

#include <inttypes.h>

namespace android {
namespace base {

// This is mainly used to implement pipe serialization of complex
// GL / Vulkan structs.
// This is copied from the AOSP
// "platform/external/qemu/android/android-emu/android/base/files/InplaceStream.h";
// In the AOSP project, there was a MemStream class, but it is inlined into InplaceStream.
// Also, this is made pre-c++11 compatible.

// P.S. The most important difference of this class versus android::base::Stream
// is that it does not allocate the stream; that's up to the user of this class
// to provide a |buffer|.
class InplaceStream {
public:
    InplaceStream(char* buffer, uint32_t buflen) :
        mBuffer(buffer),
        mBufferLen((int)buflen) { }

    // Read up to |size| bytes and copy them to |buffer|. Return the number
    // of bytes that were actually transferred, or -errno value on error.
    ssize_t read(void* buffer, size_t size);

    // Write up to |size| bytes from |buffer| into the stream. Return the
    // number of bytes that were actually transferred, or -errno value on
    // error.
    ssize_t write(const void* buffer, size_t size);

    int writtenSize() const;
    int readPos() const;
    int readSize() const;

    char* currentRead() const;
    char* currentWrite() const;
    ssize_t advanceRead(size_t size);
    ssize_t advanceWrite(size_t size);

    // Write a single byte |value| into the stream. Ignore errors.
    void putByte(uint8_t value);

    // Write a 16-bit |value| as big-endian into the stream. Ignore errors.
    void putBe16(uint16_t value);

    // Write a 32-bit |value| as big-endian into the stream. Ignore errors.
    void putBe32(uint32_t value);

    // Write a 64-bit |value| as big-endian into the stream. Ignore errors.
    void putBe64(uint64_t value);

    // Read a single byte from the stream. Return 0 on error.
    uint8_t getByte();

    // Read a single big-endian 16-bit value from the stream.
    // Return 0 on error.
    uint16_t getBe16();

    // Read a single big-endian 32-bit value from the stream.
    // Return 0 on error.
    uint32_t getBe32();

    // Read a single big-endian 64-bit value from the stream.
    // Return 0 on error.
    uint64_t getBe64();

    // Write a 32-bit float |value| to the stream.
    void putFloat(float value);

    // Read a single 32-bit float value from the stream.
    float getFloat();

    // Write a string |str| into the stream. Ignore errors.
    void putString(const std::string& str);

    // Write a 0-terminated C string |str| into the stream. Ignore error.
    void putString(const char* str);

    // Write a 0-terminated C string |str| into the stream, including
    // the last '\0'. Ignore error.
    void putStringNullTerminated(const char* str);
    const char* getStringNullTerminated();

    // Write a string |str| of |strlen| bytes into the stream.
    // Ignore errors.
    void putString(const char* str, size_t strlen);

    // Read a string from the stream. Return a new string instance,
    // which will be empty on error. Note that this can only be used
    // to read strings that were written with putString().
    std::string getString();

private:

    char* mBuffer;
    int mBufferLen;
    int mReadPos = 0;
    int mWritePos = 0;
};

}  // namespace base
}  // namespace android
