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

#include "android/snapshot/Compressor.h"

#include "android/base/system/System.h"

#include "lz4.h"

#include <algorithm>
#include <cassert>
#include <utility>

namespace android {
namespace snapshot {

namespace compress {

int workerCount() {
    return std::max(2, std::min(4, base::System::get()->getCpuCoreCount() - 1));
}

int32_t compress(const uint8_t* data,
                 int32_t size,
                 uint8_t* out,
                 int32_t outSize) {
    assert(out);
    assert(outSize >= maxCompressedSize(size));
    int32_t compressedSize =
            LZ4_compress_fast(reinterpret_cast<const char*>(data),
                              reinterpret_cast<char*>(out), size, outSize, 1);
    return compressedSize;
}

}  // namespace compress

}  // namespace snapshot
}  // namespace android
