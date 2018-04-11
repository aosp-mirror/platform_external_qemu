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
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/MemoryHints.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"

#ifdef _WIN32

#include <set>
#include <memoryapi.h>

#else
#include <sys/mman.h>
#include <sys/types.h>
#endif

namespace android {
namespace base {

#ifdef _WIN32

class MemoryReclaimer {
public:
    enum OfferPriority {
        VeryLow = 1,
        Low,
        BelowNormal,
        Normal,
    };

    using OfferVirtualMemoryFunc = DWORD (WINAPI *)(PVOID, SIZE_T, OfferPriority);
    using ReclaimVirtualMemoryFunc = DWORD (WINAPI *)(PVOID, SIZE_T);

    MemoryReclaimer() {
        HMODULE kernelLib = LoadLibrary("kernelbase");
        if (kernelLib) {
            mOfferFunc =
                reinterpret_cast<OfferVirtualMemoryFunc>(
                    GetProcAddress(kernelLib, "OfferVirtualMemory"));
            mReclaimFunc =
                reinterpret_cast<ReclaimVirtualMemoryFunc>(
                        GetProcAddress(kernelLib, "ReclaimVirtualMemory"));
        }
        mValid = kernelLib && mOfferFunc && mReclaimFunc;
    }

    bool offerRange(void* start, uint64_t length) {
        if (!mValid) return true;
        android::base::AutoLock lock(mLock);
        std::pair<void*, uint64_t> range = std::make_pair(start, length);
        mOfferedRanges.insert(range);
        return ERROR_SUCCESS == mOfferFunc((PVOID)start, (SIZE_T)length, OfferPriority::VeryLow);
    }

    // Must exactly match an existing range in mOfferedRanges, or not overlap
    // with any range at all.
    bool reclaimRange(void* start, uint64_t length) {
        if (!mValid) return true;
        android::base::AutoLock lock(mLock);
        std::pair<void*, uint64_t> range = std::make_pair(start, length);
        if (mOfferedRanges.find(range) != mOfferedRanges.end()) {
            mOfferedRanges.erase(range);
            return ERROR_SUCCESS == mReclaimFunc(range.first, range.second);
        }
        return true;
    }

private:
    android::base::Lock mLock;
    std::set<std::pair<void*, uint64_t> > mOfferedRanges;

    OfferVirtualMemoryFunc mOfferFunc = 0;
    ReclaimVirtualMemoryFunc mReclaimFunc = 0;
    bool mValid = false;

    DISALLOW_COPY_ASSIGN_AND_MOVE(MemoryReclaimer);
};

using android::base::LazyInstance;
static LazyInstance<MemoryReclaimer> sReclaimer = LAZY_INSTANCE_INIT;

#endif

bool memoryHint(void* start, uint64_t length, MemoryHint hint) {
#ifdef _WIN32
    static bool newerVirtualMemFuncsSupported =
        System::get()->getOsVersionInfo().windowsVersion >=
            System::OsVersionInfo::WindowsVersion::Win81;

    if (newerVirtualMemFuncsSupported) {
        switch (hint) {
        case MemoryHint::DontNeed:
            return sReclaimer->offerRange(start, length);
        case MemoryHint::Normal:
            return sReclaimer->reclaimRange(start, length);
        // TODO: Find some way to implement those on Windows
        case MemoryHint::Random:
        case MemoryHint::Sequential:
        default:
            return true;
        }
    } else {
        return true;
    }
#else
    int asAdviseFlag = MADV_NORMAL;
    switch (hint) {
        case MemoryHint::DontNeed:
            asAdviseFlag = MADV_DONTNEED;
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
        default:
            break;
    }

    int res = madvise(start, length, asAdviseFlag);
    return res == 0;
#endif
}

} // namespace base
} // namespace android
