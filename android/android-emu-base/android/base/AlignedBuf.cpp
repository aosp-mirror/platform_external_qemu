// Copyright 2018 The Android Open Source Project
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

#include "AlignedBuf.h"

namespace android {

// Convenience function for aligned malloc across platforms
void* aligned_buf_alloc(size_t align, size_t size) {
    size_t actualAlign = std::max(align, sizeof(void*));
#ifdef _WIN32
    void* res = _aligned_malloc(size, actualAlign);
    if (!res) {
        abort();
    }
    return res;
#else
    void* res;
    if (posix_memalign(&res, actualAlign, size)) {
        abort();
    }
    return res;
#endif
}

void aligned_buf_free(void* buf) {
#ifdef _WIN32
    _aligned_free(buf);
#else
    free(buf);
#endif
}

} // namespace android