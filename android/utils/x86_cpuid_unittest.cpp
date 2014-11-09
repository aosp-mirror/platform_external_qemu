// Copyright (c) 2014, Intel Corporation
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/x86_cpuid.h"

#include <stdio.h>

#include <gtest/gtest.h>

namespace android {
namespace utils {

// Applicable when calling CPUID with EAX=0x80000001
// Long mode, which is one of the features that distinguish x86_64 from x86
static const uint32_t CPUID_EDX_LM = 1 << 29;

// Prints to stdout a line stating whether a feature is supported or not.
static inline void PrintCpuFeatureTestResult(const char *feature,
                                             bool supported) {
    printf("%s -%s supported\n", feature, supported ? "" : " not");
}

// Not really a test. Prints a summary of the host CPU's ABI compatibility for
// manual verification.
TEST(x86_cpuid, Default) {
    uint32_t ecx = 0, edx = 0;

    // Call CPUID with EAX=1 and ECX=0 to get CPU feature bits.
    android_get_x86_cpuid(1, 0, NULL, NULL, &ecx, &edx);

    if (!ecx && !edx) {
        // Both output variables are unchanged, which probably means that
        // android_get_x86_cpuid(..) was compiled to a no-op.
        printf("Host CPU architecture is neither x86 nor x86_64!\n");
    } else {
        // EDX returned by CPUID (EAX=1) should never be zero.
        EXPECT_TRUE(edx);

        uint32_t edx2 = 0;

        // Call CPUID with EAX=0x80000001 and ECX=0 to get extended CPU feature
        // bits.
        android_get_x86_cpuid(0x80000001, 0, NULL, NULL, NULL, &edx2);

        // Long mode is a distinguishing feature of x86_64.
        bool isX86_64 = edx2 & CPUID_EDX_LM;
        printf("Host CPU architecture is %s.\n", isX86_64 ? "x86_64" : "x86");

        printf("ABI compatibility:\n");

        // See Table 1 in section 'ABI Management' of NDK Programmer's Guide
        // (Android NDK r10c) for the features required by x86/x86_64 ABIs.
        PrintCpuFeatureTestResult("MMX", edx & CPUID_EDX_MMX);
        PrintCpuFeatureTestResult("SSE", edx & CPUID_EDX_SSE);
        PrintCpuFeatureTestResult("SSE2", edx & CPUID_EDX_SSE2);
        PrintCpuFeatureTestResult("SSE3", ecx & CPUID_ECX_SSE3);
        PrintCpuFeatureTestResult("SSSE3", ecx & CPUID_ECX_SSSE3);
        if (isX86_64) {
            PrintCpuFeatureTestResult("SSE4.1", ecx & CPUID_ECX_SSE41);
            PrintCpuFeatureTestResult("SSE4.2", ecx & CPUID_ECX_SSE42);
            PrintCpuFeatureTestResult("POPCNT", ecx & CPUID_ECX_POPCNT);
        }
    }
}

}  // namespace utils
}  // namespace android
