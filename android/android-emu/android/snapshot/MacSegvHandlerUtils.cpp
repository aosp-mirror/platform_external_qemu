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
//
#include "mac_segv_handler.h"

#include <vector>
#include <stdio.h>

typedef std::pair<char*, uint64_t> MemoryRange;
typedef std::vector<MemoryRange> MemoryRanges;

static MemoryRanges sMemoryRanges;

void register_segv_handling_range(void* ptr, uint64_t size) {
    sMemoryRanges.emplace_back((char*)ptr, size);
}

bool is_ptr_registered(void* ptr) {
    const auto it =
        std::find_if(sMemoryRanges.begin(), sMemoryRanges.end(),
                     [ptr](const MemoryRange& r) {
                        char* begin = r.first;
                        uint64_t sz = r.second;
                        return (ptr >= begin) && ptr < (begin + sz);
                     });
    return it != sMemoryRanges.end();
}

void clear_segv_handling_ranges(void) {
    sMemoryRanges.clear();
}

