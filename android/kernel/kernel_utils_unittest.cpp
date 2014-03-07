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

#include "android/kernel/kernel_utils.h"

#include "android/base/String.h"
#include "android/kernel/kernel_utils_testing.h"

#include <gtest/gtest.h>

using android::base::String;

namespace android {
namespace kernel {

namespace {

class ScopedDescriptionFunc {
public:
    explicit ScopedDescriptionFunc(GetFileDescriptionFunction* file_func) {
        setFileDescriptionFunction(file_func, NULL);
    }

    ScopedDescriptionFunc(GetFileDescriptionFunction* file_func,
                          void* file_opaque) {
        setFileDescriptionFunction(file_func, file_opaque);
    }

    ~ScopedDescriptionFunc() {
        setFileDescriptionFunction(NULL, NULL);
    }
};

}  // namespace

TEST(KernelUtils, GetKernelSerialDevicePrefix) {
    EXPECT_STREQ("ttyS",
            android_kernelSerialDevicePrefix(KERNEL_TYPE_LEGACY));
    EXPECT_STREQ("ttyGF",
            android_kernelSerialDevicePrefix(KERNEL_TYPE_3_10_OR_ABOVE));
}

static bool failFunc(void* opaque, const char* path, String* text) {
    return false;
}

TEST(KernelUtils, ProbeKernelTypeWithNoKernelFile) {
    ScopedDescriptionFunc func(&failFunc);

    KernelType ktype;
    EXPECT_FALSE(android_pathProbeKernelType("/tmp/kernel", &ktype));
}

struct Expectation {
    const char* description;
    bool result;
    KernelType ktype;

    static bool getDescriptionFunc(void* opaque,
                                   const char* path,
                                   String* text) {
        const Expectation* expectation = static_cast<const Expectation*>(opaque);
        if (!expectation->result)
            return false;
        text->assign(expectation->description);
        return true;
    }
};

#ifdef _WIN32
#define DISABLED_ON_WIN32(x)  DISABLED_ ## x
#else
#define DISABLED_ON_WIN32(x)  x
#endif

TEST(KernelUtils, DISABLED_ON_WIN32(PathProbeKernelType)) {
    static const Expectation kData[] = {
        { NULL, false, KERNEL_TYPE_LEGACY },
        // Missing bzImage.
        { "Linux kernel x86 boot executable raw image, version 3.10.0+",
                false, KERNEL_TYPE_LEGACY },
        // Missing version
        { "Linux kernel x86 boot executable bzImage, 3.10.0+",
                false, KERNEL_TYPE_LEGACY },
        // Legacy 2.6.29 kernel
        { "Linux kernel x86 boot executable bzImage, version 2.6.29 (foo...)",
                true,
                KERNEL_TYPE_LEGACY },
        // Legacy 3.4
        { "Linux kernel x86 boot executable bzImage, version 3.4.1 (foo...)",
                true,
                KERNEL_TYPE_LEGACY },
        // 3.10
        { "Linux kernel x86 boot executable bzImage, version 3.10.0+",
                true,
                KERNEL_TYPE_3_10_OR_ABOVE },
        // 3.40
        { "Linux kernel x86 boot executable bzImage, version 3.40.0",
                true,
                KERNEL_TYPE_3_10_OR_ABOVE },
        // 4.0
        { "Linux kernel x86 boot executable bzImage, version 4.0.9",
                true,
                KERNEL_TYPE_3_10_OR_ABOVE },
    };
    const size_t kDataSize = sizeof(kData) / sizeof(kData[0]);
    static const char kKernelPath[] = "/tmp/kernel";
    for (size_t n = 0; n < kDataSize; ++n) {
        KernelType kernelType;
        const Expectation& expectation = kData[n];
        ScopedDescriptionFunc func(&Expectation::getDescriptionFunc,
                                   (void*)&expectation);
        EXPECT_EQ(expectation.result,
                  android_pathProbeKernelType(kKernelPath, &kernelType))
                        << "For [" << expectation.description << "]";
        if (expectation.result) {
            EXPECT_EQ(expectation.ktype, kernelType) << "For ["
                    << expectation.description << "]";
        }
    }
}

}  // namespace kernel
}  // namespace android
