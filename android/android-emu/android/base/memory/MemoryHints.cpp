// Copyright (C) 2018 The Android Open Source Project
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

#include "android/base/Compiler.h"
#include "android/base/ContiguousRangeMapper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/MemoryHints.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"

#ifndef _WIN32
#include <sys/mman.h>
#include <sys/types.h>
#endif

using android::base::ContiguousRangeMapper;
using android::base::LazyInstance;

namespace android {
namespace base {

// Assumes is aligned to page size and the whole page is there.
// Explicitly copies to an intermediate buffer---even if we emit asm
// that reads and writes the same address, it could get skipped as
// a no-op from the POV of the operating system.
static constexpr size_t kTouchBufferSize = 16 * 1048576;

class MemoryTouchBuffer {
public:

    MemoryTouchBuffer() {
        mBuffer.resize(kTouchBufferSize);
    }

    char* ptr() { return mBuffer.data(); }

private:
    std::vector<char> mBuffer;
};

static LazyInstance<MemoryTouchBuffer> sTouchBuffer = LAZY_INSTANCE_INIT;

static void rewriteMemory(void* toRewrite, uint64_t length) {

    ContiguousRangeMapper rewriter([](uintptr_t start, uintptr_t size) {
        char* staging = sTouchBuffer->ptr();

        memcpy(staging, (uint8_t*)start, size);
        memcpy((uint8_t*)start, staging, size);
        if (memcmp(staging, (uint8_t*)start, size)) {
            fprintf(stderr, "%s: goofed\n", __func__);
        }
    }, kTouchBufferSize);

    uint8_t* start = (uint8_t*)toRewrite;

    for (uint64_t i = 0; i < length; i += kTouchBufferSize) {
        rewriter.add((uintptr_t)start + i, std::min(length - i, (uint64_t)kTouchBufferSize));
    }
}

bool memoryHint(void* start, uint64_t length, MemoryHint hint) {
#ifdef _WIN32
    switch (hint) {
    case MemoryHint::DontNeed:
    case MemoryHint::PageOut:
        // https://blogs.msdn.microsoft.com/oldnewthing/20170113-00/?p=95185
        // "Around the Windows NT 4 era, a new trick arrived on the scene: You
        // could VirtualUnlock memory that was already unlocked in order to
        // remove it from your working set. This was a trick, because it took
        // what used to be a programming error and gave it additional meaning,
        // but in a way that didn't break backward compatibility because the
        // contractual behavior of the memory did not change: The contents of
        // the memory remain valid and the program is still free to access it
        // at any time. The new behavior is that unlocking unlocked memory also
        // takes it out of the process's working set, so that it becomes a
        // prime candidate for being paged out and used to satisfy another
        // memory allocation."
        VirtualUnlock(start, length);
        VirtualUnlock(start, length);
        return true;
    case MemoryHint::Touch:
        rewriteMemory(start, length);
        return true;
    case MemoryHint::Normal:
        return true;
    // TODO: Find some way to implement those on Windows
    case MemoryHint::Random:
    case MemoryHint::Sequential:
        return true;
    default:
        return true;
    }
#else // macOS and Linux

    int asAdviseFlag = MADV_NORMAL;
    bool reprotect = false;
    bool skipAdvise = false;

    switch (hint) {
        case MemoryHint::DontNeed:
#ifdef __APPLE__
            asAdviseFlag = MADV_FREE;
            // On Mac, an explicit mprotect() needs to happen to kick the page out.
            reprotect = true;
#else // Linux
            // MADV_FREE would be best, but it is not necessarily
            // supported on all Linux systems.
            asAdviseFlag = MADV_DONTNEED;
#endif // __APPLE__
            break;
        case MemoryHint::PageOut:
            // MADV_DONTNEED / MADV_FREE change the semantics of the memory,
            // so all we do here is skip the madvise call and mprotect().
            skipAdvise = true;
#ifdef __APPLE__
            reprotect = true;
#endif
            break;
        case MemoryHint::Normal:
            asAdviseFlag = MADV_NORMAL;
            break;
        case MemoryHint::Random:
            asAdviseFlag = MADV_RANDOM;
            break;
        case MemoryHint::Sequential:
            asAdviseFlag = MADV_SEQUENTIAL;
            break;
        case MemoryHint::Touch:
            rewriteMemory(start, length);
            break;
        default:
            break;
    }

    int res = 0;

    if (!skipAdvise) {
        res = madvise(start, length, asAdviseFlag);
    }

    if (reprotect) {
        mprotect(start, length, PROT_NONE);
        mprotect(start, length, PROT_READ | PROT_WRITE | PROT_EXEC);
    }

    return res == 0;
#endif // _WIN32
}

bool zeroOutMemory(void* start, uint64_t length) {
    memset(start, 0, length);
    return memoryHint(start, length, MemoryHint::DontNeed) &&
           memoryHint(start, length, MemoryHint::Normal);
}

} // namespace base
} // namespace android
