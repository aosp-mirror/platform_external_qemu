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

void Stream::putString(const String& str) {
    putString(str.c_str(), str.size());
}

void Stream::putString(const char* str) {
    putString(str, str ? ::strlen(str) : 0U);
}

void Stream::putString(const char* str, size_t len) {
    if (!str) {
        str = "";
        len = 0;
    }
    this->putBe32(len);
    this->write(str, len);
}

String Stream::getString() {
    String result;
    size_t len = this->getBe32();
    result.resize(len);
    if (this->read(&result[0], len) != static_cast<ssize_t>(len)) {
        result.resize(0);
    }
    return result;
}

}  // namespace base
}  // namespace android
