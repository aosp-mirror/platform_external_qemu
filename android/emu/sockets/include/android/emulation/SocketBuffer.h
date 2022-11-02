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
#include "aemu/base/files/Stream.h"

namespace android {
namespace emulation {

struct SocketBuffer {
    bool isEmpty() const { return mBuf.empty(); }

    void append(const void *data, size_t size) {
        if (mConsumed > 0) {
            mBuf.erase(mBuf.begin(), mBuf.begin() + mConsumed);
            mConsumed = 0;
        }

        const uint8_t *data8 = static_cast<const uint8_t *>(data);
        mBuf.insert(mBuf.end(), data8, data8 + size);
    }

    std::pair<const void*, size_t> peek() const {
        return {mBuf.data() + mConsumed, mBuf.size() - mConsumed};
    }

    void consume(size_t size) {
        mConsumed += size;
    }

    void clear() {
        mBuf.clear();
        mConsumed = 0;
    }

    void save(base::Stream* stream) const {
        const auto x = peek();
        stream->putBe32(x.second);
        stream->write(x.first, x.second);
    }

    bool load(base::Stream* stream) {
        mConsumed = 0;
        mBuf.resize(stream->getBe32());
        return stream->read(mBuf.data(), mBuf.size()) == mBuf.size();
    }

    std::vector<uint8_t> mBuf;
    size_t mConsumed = 0;
};

}  // namespace emulation
}  // namespace android
