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

// This is copied from the AOSP
// "platform/external/qemu/android/android-emu/android/base/files/InplaceStream.cpp"
#include "InplaceStream.h"

#include <string.h>

namespace android {
namespace base {

ssize_t InplaceStream::read(void* buffer, size_t size) {
    if (!size) return 0;
    size_t toRead = ((size_t)readSize() < size) ? (size_t)readSize() : size;
    memcpy(buffer, mBuffer + mReadPos, toRead);
    mReadPos += toRead;
    return toRead;
}

ssize_t InplaceStream::write(const void* buffer, size_t size) {
    if (!size) return 0;
    int capacity = mBufferLen - writtenSize();
    size_t toWrite = ((size_t)capacity < size) ? (size_t)capacity : size;
    memcpy(mBuffer + mWritePos, buffer, capacity);
    mWritePos += toWrite;
    return toWrite;
}

int InplaceStream::writtenSize() const {
    return (int)mWritePos;
}

int InplaceStream::readPos() const {
    return mReadPos;
}

int InplaceStream::readSize() const {
    return mBufferLen - mReadPos;
}

char* InplaceStream::currentRead() const {
    return mBuffer + mReadPos;
}

char* InplaceStream::currentWrite() const {
    return mBuffer + mWritePos;
}

ssize_t InplaceStream::advanceRead(size_t size) {
    const auto toAdvance = std::min<int>(size, readSize());
    mReadPos += toAdvance;
    return toAdvance;
}

ssize_t InplaceStream::advanceWrite(size_t size) {
    const auto toAdvance = std::min<int>(size, mBufferLen - writtenSize());
    mWritePos += toAdvance;
    return toAdvance;
}

void InplaceStream::putByte(uint8_t value) {
    write(&value, 1U);
}

uint8_t InplaceStream::getByte() {
    uint8_t value[1] = { 0 };
    read(value, 1U);
    return value[0];
}

void InplaceStream::putBe16(uint16_t value) {
    uint8_t b[2] = { (uint8_t)(value >> 8), (uint8_t)value };
    write(b, 2U);
}

uint16_t InplaceStream::getBe16() {
    uint8_t b[2] = { 0, 0 };
    read(b, 2U);
    return ((uint16_t)b[0] << 8) | (uint16_t)b[1];
}

void InplaceStream::putBe32(uint32_t value) {
    uint8_t b[4] = {
            (uint8_t)(value >> 24),
            (uint8_t)(value >> 16),
            (uint8_t)(value >> 8),
            (uint8_t)value };
    write(b, 4U);
}

uint32_t InplaceStream::getBe32() {
    uint8_t b[4] = { 0, 0, 0, 0 };
    read(b, 4U);
    return ((uint32_t)b[0] << 24) |
           ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8) |
           (uint32_t)b[3];
}

void InplaceStream::putBe64(uint64_t value) {
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

uint64_t InplaceStream::getBe64() {
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

void InplaceStream::putFloat(float v) {
    union {
        float f;
        uint8_t bytes[sizeof(float)];
    } u;
    u.f = v;
    this->write(u.bytes, sizeof(u.bytes));
}

float InplaceStream::getFloat() {
    union {
        float f;
        uint8_t bytes[sizeof(float)];
    } u;
    this->read(u.bytes, sizeof(u.bytes));
    return u.f;
}

void InplaceStream::putString(const std::string& str) {
    putString(str.c_str(), str.size());
}

void InplaceStream::putString(const char* str) {
    putString(str, str ? ::strlen(str) : 0U);
}

// Is null-terminated to double as plain C strings
void InplaceStream::putStringNullTerminated(const char* str) {
    putString(str, str ? ::strlen(str) + 1 : 0U);
}

void InplaceStream::putString(const char* str, size_t len) {
    if (!str) {
        str = "";
        len = 0;
    }
    this->putBe32(len);
    this->write(str, len);
}

std::string InplaceStream::getString() {
    std::string result;
    size_t len = this->getBe32();
    if (len > 0) {
        result.resize(len);
        if (this->read(&result[0], len) != static_cast<ssize_t>(len)) {
            result.clear();
        }
    }
    return result;
}

const char* InplaceStream::getStringNullTerminated() {
    size_t len = this->getBe32();
    const char* res = nullptr;
    if (len > 0) {
        res = currentRead();
        advanceRead(len);
    }
    return res;
}

}  // namespace base
}  // namespace android
