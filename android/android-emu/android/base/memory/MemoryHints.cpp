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

#include "android/base/memory/MemoryHints.h"
#include "android/base/system/System.h"

#ifdef _WIN32
#else
#include <sys/mman.h>
#include <sys/types.h>
#endif

namespace android {
namespace base {

// static
bool MemoryHints::set(void* start, uint64_t length, Hint hint) {
#ifdef _WIN32
    bool newerVirtualMemFuncsSupported =
        System::get()->getOsVersionInfo()->windowsVersion >=
            System::OsVersionInfo::WindowsVersion::Win81;

    fprintf(stderr, "%s: newer funcs supported!\n", __func__);

    // TODO: Windows
    return true;
#else
    int asAdviseFlag = MADV_NORMAL;
    switch (hint) {
        case Hint::DontNeed:
            asAdviseFlag = MADV_DONTNEED;
            break;
        case Hint::Normal:
            asAdviseFlag = MADV_NORMAL;
            break;
        case Hint::Random:
            asAdviseFlag = MADV_RANDOM;
            break;
        case Hint::Sequential:
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
