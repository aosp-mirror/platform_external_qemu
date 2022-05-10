// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/uncompress.h"

#include "zconf.h"  // for Bytef
#include "zlib.h"   // for z_stream, inflateEnd, inflate, Z_OK, Z_FINISH

auto uncompress_gzipStream(uint8_t* dst,
                           size_t* dstLen,
                           const uint8_t* src,
                           size_t srcLen) -> bool {
    z_stream stream;
    stream.next_in = const_cast<Bytef*>(src);
    stream.avail_in = srcLen;
    stream.next_out = dst;
    stream.avail_out = *dstLen;
    stream.zalloc = (alloc_func) nullptr;
    stream.zfree = (free_func) nullptr;

    // magic number from gz_read
    const int GZIP_WINDOW_BITS = 15 + 16;

    int result = inflateInit2(&stream, GZIP_WINDOW_BITS);
    if (result != Z_OK) {
        return result != 0;
    }

    result = inflate(&stream, Z_FINISH);
    *dstLen = stream.total_out;
    if (result == Z_STREAM_END) {
        result = inflateEnd(&stream);
    } else {
        // preserve error code
        inflateEnd(&stream);
    }
    return result == Z_OK;
}
