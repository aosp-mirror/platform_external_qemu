/* Copyright (C) 2009 The Android Open Source Project
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
#ifndef ANDROID_UTILS_UNCOMPRESS_H
#define ANDROID_UTILS_UNCOMPRESS_H

#include "android/utils/compiler.h"

#include <stdint.h>
#include <stddef.h>

ANDROID_BEGIN_HEADER

// uncompress a gzip file in memory into memory in one function call
// the dstLen must be large enough to hold all the decompressed data
// 
// src - pointer to the beginning of the gzip file data
// srcLen - total number of bytes in the gzip file
// dst - pointer that receives the decompressed bytes
// dstLen - number of bytes available for decompressed data
//
// return values (defined in zlib.h)
// Z_OK - all data decompressed correctly, dstLen hold the number of 
// bytes decompressed
// Z_BUF_ERROR - dst buffer too small, dstLen hold the number of 
// bytes decompressed
// Z_DATA_ERROR - error in compressed data stream
// Z_MEM_ERROR - out of memory error prevented completion
int uncompress_gzipStream(uint8_t* dst, size_t* dstLen, const uint8_t* src,
                          size_t srcLen);

ANDROID_END_HEADER

#endif /* ANDROID_UTILS_UNCOMPRESS_H */
