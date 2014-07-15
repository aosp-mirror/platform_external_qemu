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

#include "android/base/containers/PodVector.h"
#include "android/base/Limits.h"
#include "android/base/Log.h"
#include "android/kernel/kernel_utils.h"
#include "android/kernel/kernel_utils_testing.h"
#include "android/utils/file_data.h"
#include "android/utils/path.h"
#include "android/utils/uncompress.h"
#include "zlib.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define DEBUG_KERNEL  0

#define KERNEL_LOG     LOG_IF(INFO, DEBUG_KERNEL)
#define KERNEL_PLOG    PLOG_IF(INFO, DEBUG_KERNEL)
#define KERNEL_ERROR   LOG_IF(ERROR, DEBUG_KERNEL)
#define KERNEL_PERROR  PLOG_IF(ERROR, DEBUG_KERNEL)

using android::base::PodVector;

namespace android {
namespace kernel {

// TODO: (vharron) move somewhere more generally useful?
// Returns a pointer to the first occurrence of |data2| in |data1|, or a NULL
// pointer if |data2| is not part of |data1|.
// very similar to a binary strstr
static const uint8_t* findBinarySequence(const uint8_t* data1, size_t size1,
                                         const uint8_t* data2, size_t size2) {
    if (size1 < size2) {
        return NULL;
    }
    for (size_t pos1 = 0; pos1 <= size1 - size2; pos1++) {
        size_t pos2;
        for (pos2 = 0; pos2 < size2; pos2++) {
            if (data1[pos1 + pos2] != data2[pos2]) {
                break;
            }
        }
        if (pos2 == size2) {
            // all characters matched, return a pointer to the beginning of the
            // sequence
            return data1 + pos1;
        }
    }
    return NULL;
}

static const char* VersionStringPrefix = "Linux version ";
static const size_t VersionStringPrefixLen = strlen(VersionStringPrefix);

}  // namespace kernel
}  // namespace android

bool android_parseLinuxVersionString(const char* versionString,
                                     KernelVersion* kernelVersion) {
    if (!strstr(versionString, android::kernel::VersionStringPrefix)) {
        KERNEL_ERROR << "unsupported kernel version string:" << versionString;
        return false;
    }
    // skip past the prefix to the version number
    versionString += android::kernel::VersionStringPrefixLen;

    uint32_t temp = 0;
    for (int i = 0; i < 3; i++) {
        // skip '.' delimeters
        while (i > 0 && *versionString == '.') {
            versionString++;
        }

        char* end;
        unsigned long number = ::strtoul(versionString, &end, 10);
        if (end == versionString || number > 0xff) {
            KERNEL_ERROR << "unsupported kernel version string:"
                         << versionString;
            return false;
        }
        temp <<= 8;
        temp |= number;
        versionString = end;
    }
    *kernelVersion = (KernelVersion)temp;

    char kernelVersionHex[32];
    sprintf(kernelVersionHex, "0x%06x", temp);
    KERNEL_LOG << "Kernel version hex: " << kernelVersionHex;
    return true;
}

const char* android_kernelSerialDevicePrefix(KernelVersion kernelVersion) {
    if (kernelVersion >= KERNEL_VERSION_3_10_0) {
        return "ttyGF";
    }
    return "ttyS";
}

bool android_imageProbeKernelVersionString(const uint8_t* kernelFileData,
                                           size_t kernelFileSize,
                                           char* dst/*[dstLen]*/,
                                           size_t dstLen) {
	PodVector<uint8_t> uncompressed;

	const uint8_t* uncompressedKernel = NULL;
	size_t uncompressedKernelLen = 0;

	const char kElfHeader[] = { 0x7f, 'E', 'L', 'F' };

    if (kernelFileSize < sizeof(kElfHeader)) {
        KERNEL_ERROR << "Kernel image too short";
        return false;
    }

	if (0 == memcmp(kElfHeader, kernelFileData, 4)) {
		// this is an uncompressed ELF file (probably mips)
		uncompressedKernel = kernelFileData;
		uncompressedKernelLen = kernelFileSize;
	}
	else {
		// handle compressed kernels here
		const uint8_t kGZipMagic[4] = { 0x1f, 0x8b, 0x08, 0x00 };
		const uint8_t* compressedKernel =
            android::kernel::findBinarySequence(kernelFileData,
                                                kernelFileSize,
                                                (const uint8_t*)kGZipMagic,
                                                sizeof(kGZipMagic));
		if (!compressedKernel) {
			KERNEL_ERROR << "Could not find gzip header in kernel file!";
			return false;
		}

        size_t compressedKernelLen = kernelFileSize -
            (compressedKernel - kernelFileData);

        // inflate ratios for all prebuilt kernels on 2014-07-14 is 1.9:1 ~
        // 3.43:1 not sure how big the uncompressed size is, so make an
        // absurdly large buffer
        uncompressedKernelLen = compressedKernelLen * 10;
        uncompressed.resize(uncompressedKernelLen);
        uncompressedKernel = uncompressed.begin();

        int zResult = uncompress_gzipStream(uncompressed.begin(),
                                            &uncompressedKernelLen,
                                            compressedKernel,
                                            compressedKernelLen);
		if (zResult == Z_DATA_ERROR) {
			KERNEL_ERROR << "Compressed kernel image is corrupt?";
			return false;
		}
        else if (zResult == Z_MEM_ERROR) {
            KERNEL_ERROR << "Not enough memory to decompress kernel!";
            return false;
        }
        else if (zResult == Z_BUF_ERROR) {
            KERNEL_ERROR << "Kernel decompression destination buffer too small"
                            "to hold entire kernel!";
            // it was partially decompressed, so we're going to try to find
            // the version string anyway
        }
    }

    // okay, now we have a pointer to an uncompressed kernel, let's find the
    // version string
    const uint8_t* versionStringStart =
        android::kernel::findBinarySequence(
            uncompressedKernel,
            uncompressedKernelLen,
            (const uint8_t*)android::kernel::VersionStringPrefix,
            android::kernel::VersionStringPrefixLen);
    if (!versionStringStart) {
		KERNEL_ERROR << "Could not find 'Linux version ' in kernel!";
		return false;
	}

    strlcpy(dst, (const char*)versionStringStart, dstLen);

	return true;
}

bool android_pathProbeKernelVersionString(const char* kernelPath,
                                          char* dst/*[dstLen]*/,
                                          size_t dstLen) {
    FileData kernelFileData;
    if (0 < fileData_initFromFile(&kernelFileData, kernelPath)) {
        KERNEL_ERROR << "Could not open kernel file!";
        return false;
    }

    return android_imageProbeKernelVersionString(kernelFileData.data,
                                                 kernelFileData.size,
                                                 dst,
                                                 dstLen);
}
