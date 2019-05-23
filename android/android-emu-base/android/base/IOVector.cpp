// Copyright 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/base/IOVector.h"

#include <algorithm>
#include <cstring>

namespace android {
namespace base {

size_t IOVector::copyTo(void* destination, size_t offset, size_t size) {
    iovec_lookup lookup = lookup_iovec(offset);
    if (lookup.iov_index == mIOVecs.size()) {
        return 0;
    }
    int iov_index = lookup.iov_index;
    size_t current_offset = lookup.current_offset;
    size_t remaining = size;
    uint8_t* current_destination = static_cast<uint8_t*>(destination);
    size_t write_offset = offset - current_offset;
    while (remaining && iov_index < mIOVecs.size()) {
        size_t iov_len = mIOVecs[iov_index].iov_len;
        size_t to_copy = std::min(remaining, iov_len - write_offset);
        memcpy(current_destination,
               &(static_cast<uint8_t*>(
                       mIOVecs[iov_index].iov_base)[write_offset]),
               to_copy);
        write_offset = 0;
        current_destination += to_copy;
        remaining -= to_copy;
        ++iov_index;
    }
    return size - remaining;
}

size_t IOVector::copyFrom(const void* source, size_t offset, size_t size) {
    iovec_lookup lookup = lookup_iovec(offset);
    if (lookup.iov_index == mIOVecs.size()) {
        return 0;
    }
    int iov_index = lookup.iov_index;
    size_t current_offset = lookup.current_offset;
    size_t remaining = size;
    size_t write_offset = offset - current_offset;
    const uint8_t* current_source = static_cast<const uint8_t*>(source);
    while (remaining && iov_index < mIOVecs.size()) {
        size_t iov_len = mIOVecs[iov_index].iov_len;
        size_t to_copy = std::min(remaining, iov_len - write_offset);
        memcpy(&(static_cast<uint8_t*>(
                       mIOVecs[iov_index].iov_base)[write_offset]),
               current_source, to_copy);
        write_offset = 0;
        current_source += to_copy;
        remaining -= to_copy;
        ++iov_index;
    }
    return size - remaining;
}

size_t IOVector::appendEntriesTo(IOVector* destination,
                                 size_t offset,
                                 size_t size) {
    iovec_lookup lookup = lookup_iovec(offset);
    if (lookup.iov_index == mIOVecs.size()) {
        return 0;
    }
    int iov_index = lookup.iov_index;
    size_t current_offset = lookup.current_offset;
    size_t remaining = size;
    size_t write_offset = offset - current_offset;
    while (remaining && iov_index < mIOVecs.size()) {
        size_t iov_len = mIOVecs[iov_index].iov_len;
        size_t to_copy = std::min(remaining, iov_len - write_offset);
        struct iovec iov;
        iov.iov_base = &(static_cast<uint8_t*>(
                mIOVecs[iov_index].iov_base)[write_offset]);
        iov.iov_len = to_copy;
        destination->push_back(iov);
        write_offset = 0;
        remaining -= to_copy;
        ++iov_index;
    }
    return size - remaining;
}

IOVector::iovec_lookup IOVector::lookup_iovec(size_t offset) const {
    iovec_lookup lookup = {};
    while (lookup.iov_index < mIOVecs.size()) {
        size_t iov_len = mIOVecs[lookup.iov_index].iov_len;
        if (offset >= lookup.current_offset &&
            offset < (lookup.current_offset + iov_len)) {
            break;
        }
        lookup.current_offset += iov_len;
        ++lookup.iov_index;
    }
    return lookup;
}

}  // namespace base
}  // namespace android
