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

#pragma once

#include "android/base/containers/SmallVector.h"
#include "android/base/files/MemStream.h"
#include "android/base/files/Stream.h"
#include "android/base/TypeTraits.h"

#include <string>
#include <vector>

namespace android {
namespace base {

//
// Save/load operations for different types.
//

void saveStream(Stream* stream, const MemStream& memStream);
void loadStream(Stream* stream, MemStream* memStream);

void saveBufferRaw(Stream* stream, char* buffer, uint32_t len);
bool loadBufferRaw(Stream* stream, char* buffer);

template <class T, class = enable_if<std::is_standard_layout<T>>>
void saveBuffer(Stream* stream, const std::vector<T>& buffer) {
    stream->putBe32(buffer.size());
    stream->write(buffer.data(), sizeof(T) * buffer.size());
}

template <class T, class = enable_if<std::is_standard_layout<T>>>
bool loadBuffer(Stream* stream, std::vector<T>* buffer) {
    auto len = stream->getBe32();
    buffer->resize(len);
    int ret = (int)stream->read(buffer->data(), len * sizeof(T));
    return ret == len * sizeof(T);
}

template <class T, class = enable_if<std::is_standard_layout<T>>>
void saveBuffer(Stream* stream, const SmallVector<T>& buffer) {
    stream->putBe32(buffer.size());
    stream->write(buffer.data(), sizeof(T) * buffer.size());
}

template <class T, class = enable_if<std::is_standard_layout<T>>>
bool loadBuffer(Stream* stream, SmallVector<T>* buffer) {
    auto len = stream->getBe32();
    buffer->clear();
    buffer->resize_noinit(len);
    int ret = (int)stream->read(buffer->data(), len * sizeof(T));
    return ret == len * sizeof(T);
}

template <class T, class SaveFunc>
void saveBuffer(Stream* stream, const std::vector<T>& buffer, SaveFunc&& saver) {
    stream->putBe32(buffer.size());
    for (const auto& val : buffer) {
        saver(stream, val);
    }
}

template <class T>
void saveBuffer(Stream* stream, const T* buffer, size_t numElts) {
    stream->putBe32(numElts);
    stream->write(buffer, sizeof(T) * numElts);
}

template <class T>
void loadBufferPtr(Stream* stream, T* out) {
    auto len = stream->getBe32();
    stream->read(out, len * sizeof(T));
}

template <class T, class LoadFunc>
void loadBuffer(Stream* stream, std::vector<T>* buffer, LoadFunc&& loader) {
    auto len = stream->getBe32();
    buffer->clear();
    buffer->reserve(len);
    for (uint32_t i = 0; i < len; i++) {
        buffer->emplace_back(loader(stream));
    }
}

template <class Collection, class SaveFunc>
void saveCollection(Stream* stream, const Collection& c, SaveFunc&& saver) {
    stream->putBe32(c.size());
    for (const auto& val : c) {
        saver(stream, val);
    }
}

template <class Collection, class LoadFunc>
void loadCollection(Stream* stream, Collection* c, LoadFunc&& loader) {
    const int size = stream->getBe32();
    for (int i = 0; i < size; ++i) {
        c->emplace(loader(stream));
    }
}

void saveStringArray(Stream* stream, const char* const* strings, uint32_t count);
std::vector<std::string> loadStringArray(Stream* stream);

}  // namespace base
}  // namespace android
