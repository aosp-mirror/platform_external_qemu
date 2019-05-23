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

#include "android/base/files/Stream.h"

#include <assert.h>
#include <string.h>

namespace android {
namespace base {

void Stream::putByte(uint8_t value) {
    write(&value, 1U);
}

uint8_t Stream::getByte() {
    uint8_t value[1] = { 0 };
    read(value, 1U);
    return value[0];
}

void Stream::putBe16(uint16_t value) {
    uint8_t b[2] = { (uint8_t)(value >> 8), (uint8_t)value };
    write(b, 2U);
}

uint16_t Stream::getBe16() {
    uint8_t b[2] = { 0, 0 };
    read(b, 2U);
    return ((uint16_t)b[0] << 8) | (uint16_t)b[1];
}

void Stream::putBe32(uint32_t value) {
    uint8_t b[4] = {
            (uint8_t)(value >> 24),
            (uint8_t)(value >> 16),
            (uint8_t)(value >> 8),
            (uint8_t)value };
    write(b, 4U);
}

uint32_t Stream::getBe32() {
    uint8_t b[4] = { 0, 0, 0, 0 };
    read(b, 4U);
    return ((uint32_t)b[0] << 24) |
           ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8) |
           (uint32_t)b[3];
}

void Stream::putBe64(uint64_t value) {
    uint8_t b[8] = {
            (uint8_t)(value >> 56),
            (uint8_t)(value >> 48),
            (uint8_t)(value >> 40),
            (uint8_t)(value >> 32),
            (uint8_t)(value >> 24),
            (uint8_t)(value >> 16),
            (uint8_t)(value >> 8),
            (uint8_t)value };
    write(b, 8U);
}

uint64_t Stream::getBe64() {
    uint8_t b[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    read(b, 8U);
    return ((uint64_t)b[0] << 56) |
           ((uint64_t)b[1] << 48) |
           ((uint64_t)b[2] << 40) |
           ((uint64_t)b[3] << 32) |
           ((uint64_t)b[4] << 24) |
           ((uint64_t)b[5] << 16) |
           ((uint64_t)b[6] << 8) |
           (uint64_t)b[7];
}

void Stream::putFloat(float v) {
    union {
        float f;
        uint8_t bytes[sizeof(float)];
    } u;
    u.f = v;
    this->write(u.bytes, sizeof(u.bytes));
}

float Stream::getFloat() {
    union {
        float f;
        uint8_t bytes[sizeof(float)];
    } u;
    this->read(u.bytes, sizeof(u.bytes));
    return u.f;
}

void Stream::putString(StringView str) {
    this->putBe32(str.size());
    this->write(str.data(), str.size());
}

void Stream::putString(const char* str) {
    putString(StringView(str));
}

void Stream::putString(const char* str, size_t len) {
    putString(StringView(str, len));
}

std::string Stream::getString() {
    std::string result;
    size_t len = this->getBe32();
    if (len > 0) {
        result.resize(len);
        if (this->read(&result[0], len) != static_cast<ssize_t>(len)) {
            result.clear();
        }
    }
#ifdef _WIN32
    else {
        // std::string in GCC's STL still uses copy on write implementation
        // with a single shared buffer for an empty string. Its dtor has
        // a check for that shared buffer, and it deallocates memory only if
        // the current string's instance address != shared empty string address
        // Unfortunately, in Windows DLLs each DLL has its own copy of this
        // empty string (that's just the way Windows DLLs work), so if this
        // code creates an empty string and passes it over into another module,
        // that module's std::string::~string() will compare address with its
        // empty string object, find that they are different and will try to
        // free() a static object.
        // To mitigate it we make sure the string allocates something, so it
        // isn't empty internally and dtor is OK to delete the storage.
        result.reserve(1);
    }
#endif
    return result;
}

void Stream::putPackedNum(uint64_t num) {
    do {
        auto byte = uint8_t(num & 0x7f);
        num >>= 7;
        if (num) {
            byte |= 0x80;
        }
        putByte(byte);
    } while (num != 0);
}

uint64_t Stream::getPackedNum() {
    uint64_t res = 0;
    uint8_t byte;
    int i = 0;
    do {
        byte = getByte();
        res |= uint64_t(byte & 0x7f) << (i++ * 7);
    } while (byte & 0x80 && i < 10);
    return res;
}

void Stream::putPackedSignedNum(int64_t num) {
    if (num >= 0) {
        assert((uint64_t(num) & (1ULL << 63)) == 0);
        putPackedNum(uint64_t(num) << 1);
    } else {
        assert((uint64_t(-num) & (1ULL << 63)) == 0);
        putPackedNum((uint64_t(-num) << 1) | 1);
    }
}

int64_t Stream::getPackedSignedNum() {
    auto num = getPackedNum();
    auto sign = num & 1;
    return sign ? -int64_t(num >> 1) : (num >> 1);
}

}  // namespace base
}  // namespace android
