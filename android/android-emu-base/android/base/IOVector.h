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

#pragma once

#include "android/base/Compiler.h"

#include <cstddef>
#include <numeric>
#include <vector>

#if defined(_WIN32)
/*Because android::base::IOVector has its own definition of
 struct iovec on Windows, we need to avoid the re-definition of iovec
 from qemu/osdep.h by declaring CONFIG_IOVEC beforehand. */
#ifndef CONFIG_IOVEC
#define CONFIG_IOVEC 1
/* Structure for scatter/gather I/O.  */
struct iovec {
    void* iov_base; /* Pointer to data.  */
    size_t iov_len; /* Length of data.  */
};
#endif
#else
#include <sys/uio.h>
#endif
namespace android {
namespace base {
// A thin wrapper over a vector of struct iovec.
// The client of this class is responsible for freeing up
// the memory allocated in iov_base.
class IOVector {
public:
    using iterator = struct iovec*;
    using const_iterator = const struct iovec*;
    IOVector() {}
    IOVector(iterator begin, iterator end) : mIOVecs(begin, end) {}
    // STL-style container methods.
    void push_back(const struct iovec& iov) { mIOVecs.push_back(iov); }

    struct iovec& operator[](size_t n) {
        return mIOVecs[n];
    }

    const struct iovec& operator[](size_t n) const { return mIOVecs[n]; }

    const size_t size() const { return mIOVecs.size(); }
    // clear() doesn't free the memory pointed by iov_base.
    void clear() { mIOVecs.clear(); }

    iterator begin() { return mIOVecs.data(); }

    const_iterator begin() const { return mIOVecs.data(); }

    iterator end() { return mIOVecs.data() + size(); }

    const_iterator end() const { return mIOVecs.data() + size(); }
    // Copy data from IOVector to destination, starting at the
    // offset in IOVector with the specified size.
    // Return the number of bytes copied.
    size_t copyTo(void* destination, size_t offset, size_t size);
    // Copy data to IOVector from source, starting at the offset
    // in IOVector and copying specified size.
    // Return the number of bytes copied.
    size_t copyFrom(const void* source, size_t offset, size_t size);
    // Append new iovecs from this IOVector to destination IOVector,
    // starting at the offset in this IOVector.
    // Return the number of bytes capacity added to the destination. .
    size_t appendEntriesTo(IOVector* destination, size_t offset, size_t size);

    size_t summedLength() const {
        return std::accumulate(
                mIOVecs.begin(), mIOVecs.end(), size_t(0),
                [](size_t a, const struct iovec& b) { return a + b.iov_len; });
    }

private:
    struct iovec_lookup {
        int iov_index = 0;
        size_t current_offset = 0;
    };

    // Internal helper function to return the index and current offset of the
    // iovec we look for. If the offset is out of range, the return iov_index is
    // equal to the size of the iovecs.
    iovec_lookup lookup_iovec(size_t offset) const;
    std::vector<struct iovec> mIOVecs;
};

}  // namespace base
}  // namespace android
