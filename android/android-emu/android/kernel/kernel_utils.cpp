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

#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include "android/kernel/kernel_utils.h"
#include "android/kernel/kernel_utils_testing.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/utils/path.h"

using android::base::ScopedFd;
using android::base::makeCustomScopedPtr;
using android::base::System;

namespace {
unsigned read16le(const void* p) {
    const uint8_t* u8 = static_cast<const uint8_t*>(p);
    return u8[0] | (u8[1] << CHAR_BIT);
}
}  // namespace

const char* android_kernelSerialDevicePrefix(KernelVersion kernelVersion) {
    if (kernelVersion >= KERNEL_VERSION_3_10_0) {
        return "ttyGF";
    }
    return "ttyS";
}

bool android_getKernelVersionImpl(const char* const kernelBits, size_t size,
                                  KernelVersion* kernelVersion) {
    constexpr unsigned kOffest1 = 526;
    constexpr unsigned kOffest2 = 512;
    constexpr unsigned kVersionSize = 100;

    if (size < (kOffest1 + sizeof(uint16_t))) {
        return false;
    }

    const unsigned versionOffset = read16le(kernelBits + kOffest1) + kOffest2;
    if (size < versionOffset + kVersionSize) {
        return false;
    }

    int major;
    int minor;
    int patch;
    if (sscanf(kernelBits + versionOffset, "%d.%d.%d", &major, &minor, &patch) != 3) {
        return false;
    }

    if (major < 0 || major > 255) {
        return false;
    }
    if (minor < 0 || minor > 255) {
        return false;
    }
    if (patch < 0 || patch > 255) {
        return false;
    }

    *kernelVersion = static_cast<KernelVersion>((major) << 16 | (minor << 8) | patch);
    return true;
}

bool android_getKernelVersion(const char* kernelPath,
                              KernelVersion* kernelVersion) {

    const auto kernelFile =
            ScopedFd(path_open(kernelPath, O_RDONLY | O_BINARY, 0755));

    System::FileSize size;
    if (!System::get()->fileSize(kernelFile.get(), &size)) {
        return false;
    }

    const auto kernelFileMap = android::base::makeCustomScopedPtr(
            mmap(nullptr, size, PROT_READ, MAP_PRIVATE, kernelFile.get(), 0),
            [size](void* ptr) {
                if (ptr != MAP_FAILED)
                    munmap(ptr, size);
            });
    if (!kernelFileMap || kernelFileMap.get() == MAP_FAILED) {
        return false;
    }

    return android_getKernelVersionImpl(static_cast<const char*>(kernelFileMap.get()),
                                        size, kernelVersion);
}
