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

#include "android/base/files/StreamSerializing.h"

namespace android {
namespace base {

void saveStream(Stream* stream, const MemStream& memStream) {
    memStream.save(stream);
}

void loadStream(Stream* stream, MemStream* memStream) {
    memStream->load(stream);
}

void saveBufferRaw(Stream* stream, char* buffer, uint32_t len) {
    stream->putBe32(len);
    stream->write(buffer, len);
}

bool loadBufferRaw(Stream* stream, char* buffer) {
    auto len = stream->getBe32();
    int ret = (int)stream->read(buffer, len);
    return ret == (int)len;
}

void saveStringArray(Stream* stream, const char* const* strings, uint32_t count) {
    stream->putBe32(count);
    for (uint32_t i = 0; i < count; ++i) {
        stream->putString(strings[i]);
    }
}

std::vector<std::string> loadStringArray(Stream* stream) {
    uint32_t count = stream->getBe32();
    std::vector<std::string> res;
    for (uint32_t i = 0; i < count; ++i) {
        res.push_back(stream->getString());
    }
    return res;
}

}  // namespace base
}  // namespace android
