/* Copyright 2021 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#pragma once

#include <stdint.h>
#include "android/base/files/Stream.h"

namespace android {
namespace emulation {

struct SocketBuffer {
    bool isEmpty() const { return mBuf.empty(); }

    void append(const void *data, size_t size) {
        const uint8_t *data8 = static_cast<const uint8_t *>(data);
        mBuf.insert(mBuf.end(), data8, data8 + size);
    }

    std::pair<const void*, size_t> peek() const {
        return {mBuf.data(), mBuf.size()};
    }

    void consume(size_t size) {
        mBuf.erase(mBuf.begin(), mBuf.begin() + size);
    }

    void clear() {
        mBuf.clear();
    }

    void save(base::Stream* stream) const {
        stream->putBe32(mBuf.size());
        stream->write(mBuf.data(), mBuf.size());
    }

    bool load(base::Stream* stream) {
        mBuf.resize(stream->getBe32());
        return stream->read(mBuf.data(), mBuf.size()) == mBuf.size();
    }

    std::vector<uint8_t> mBuf;
};

}  // namespace emulation
}  // namespace android
