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
#include <string.h>
#include "android/kernel/kernel_utils.h"
#include "android/kernel/kernel_utils_testing.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/system/System.h"
#include "android/uncompress.h"
#include "android/utils/path.h"

using android::base::ScopedFd;
using android::base::makeCustomScopedPtr;
using android::base::System;

namespace {
unsigned read16le(const void* p) {
    const uint8_t* u8 = static_cast<const uint8_t*>(p);
    return u8[0] | (u8[1] << CHAR_BIT);
}

bool android_parseKernelVersion(const char* str, KernelVersion* kernelVersion) {
    int major;
    int minor;
    int patch;
    if (sscanf(str, "%d.%d.%d", &major, &minor, &patch) != 3) {
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
}  // namespace

const char* android_kernelSerialDevicePrefix(KernelVersion kernelVersion) {
    if (kernelVersion >= KERNEL_VERSION_3_10_0) {
        return "ttyGF";
    }
    return "ttyS";
}

bool android_getKernelVersionOffset(const char* const kernelBits, size_t size,
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

    return android_parseKernelVersion(kernelBits + versionOffset, kernelVersion);
}

bool android_getKernelVersionPrefix(const char* const prefix,
                                    const char* kernelBits,
                                    const size_t size,
                                    KernelVersion* kernelVersion) {
    const int prefixLen = strlen(prefix);

    const char* const kernelBitsEnd = kernelBits + size;
    while (kernelBits < kernelBitsEnd) {
        const char* match = static_cast<const char*>(
            memmem(kernelBits, kernelBitsEnd - kernelBits, prefix, prefixLen));

        if (!match) {
            return false;
        } else if (android_parseKernelVersion(match + prefixLen, kernelVersion)) {
            return true;
        } else {
            kernelBits = match + 1;
        }
    }

    return false;
}

bool android_getKernelVersionSimple(const char* const kernelBits,
                                    const size_t kernelBitsSize,
                                    KernelVersion* kernelVersion) {
    if (android_getKernelVersionOffset(kernelBits, kernelBitsSize, kernelVersion)) {
        return true;
    }

    if (android_getKernelVersionPrefix("Linux version ",
                                       kernelBits, kernelBitsSize,
                                       kernelVersion)) {
        return true;
    }

    return false;
}

bool android_getKernelVersionDecompress(const char* const kernelBits,
                                        const size_t kernelBitsSize,
                                        KernelVersion* kernelVersion) {
    static const uint8_t kGZipMagic[] = {0x1f, 0x8b, 0x08, 0x00};

    const char* compressedKernelBits =
       static_cast<const char*>(memmem(kernelBits, kernelBitsSize,
                                       kGZipMagic, sizeof(kGZipMagic)));
    if (!compressedKernelBits) {
        return false;
    }

    const char* const kernelBitsEnd = kernelBits + kernelBitsSize;
    const size_t compressedKernelSize = kernelBitsEnd - compressedKernelBits;
    std::vector<char> decompressedBits(compressedKernelSize * 10);
    size_t decompressedSize = decompressedBits.size();

    const bool zOk = uncompress_gzipStream(
        reinterpret_cast<uint8_t*>(decompressedBits.data()), &decompressedSize,
        reinterpret_cast<const uint8_t*>(compressedKernelBits), compressedKernelSize);
    if (!zOk) {
        return false;
    }

    decompressedBits.resize(decompressedSize);

    return android_getKernelVersionSimple(decompressedBits.data(),
                                          decompressedBits.size(),
                                          kernelVersion);
}

bool android_getKernelVersionImpl(const char* const kernelBits,
                                  const size_t kernelBitsSize,
                                  KernelVersion* kernelVersion) {
    if (android_getKernelVersionSimple(kernelBits, kernelBitsSize, kernelVersion)) {
        return true;
    }

    if (android_getKernelVersionDecompress(kernelBits, kernelBitsSize, kernelVersion)) {
        return true;
    }

    return false;
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
