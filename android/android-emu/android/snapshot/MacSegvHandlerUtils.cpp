#include "mac_segv_handler.h"

#include <vector>
#include <stdio.h>

typedef std::pair<char*, uint64_t> MemoryRange;
typedef std::vector<MemoryRange> MemoryRanges;

static MemoryRanges sMemoryRanges;

void register_segv_handling_range(void* ptr, uint64_t size) {
    fprintf(stderr, "%s: [%p %p]\n", __func__, ptr, (char*)ptr + (uintptr_t)size);
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

