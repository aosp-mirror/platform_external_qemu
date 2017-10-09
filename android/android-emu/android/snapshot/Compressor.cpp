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
#include <utility>

namespace android {
namespace snapshot {

int CompressorBase::workerCount() {
    return std::max(2, std::min(4, base::System::get()->getCpuCoreCount() - 1));
}

std::pair<uint8_t*, int32_t> CompressorBase::compress(const uint8_t* data,
                                                      int32_t size) {
    auto outSize = LZ4_COMPRESSBOUND(size);
    auto out = new uint8_t[size_t(outSize)];
    int32_t compressedSize =
            LZ4_compress_fast(reinterpret_cast<const char*>(data),
                              reinterpret_cast<char*>(out), size, outSize, 1);
    return {out, compressedSize};
}

}  // namespace snapshot
}  // namespace android
