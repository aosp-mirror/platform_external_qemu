/*
** Copyright (c) 2014, Intel Corporation
** Copyright (c) 2015, Google, Inc.
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/utils/x86_cpuid.h"
#include <string.h>

uint32_t android_get_x86_cpuid_function_max()
{
    uint32_t function_max;
    android_get_x86_cpuid(0, 0, &function_max, 0, 0, 0);
    return function_max;
}

uint32_t android_get_x86_cpuid_extended_function_max()
{
    uint32_t function_max;
    android_get_x86_cpuid(0x80000000, 0, &function_max, 0, 0, 0);
    return function_max;
}

void android_get_x86_cpuid(uint32_t function,
                           uint32_t count,
                           uint32_t *eax,
                           uint32_t *ebx,
                           uint32_t *ecx,
                           uint32_t *edx) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t vec[4];

#ifdef __x86_64__
    asm volatile("cpuid"
                 : "=a"(vec[0]), "=b"(vec[1]),
                   "=c"(vec[2]), "=d"(vec[3])
                 : "0"(function), "c"(count) : "cc");
#else /* !__x86_64__ */
    asm volatile("pusha \n\t"
                 "cpuid \n\t"
                 "mov %%eax, 0(%2) \n\t"
                 "mov %%ebx, 4(%2) \n\t"
                 "mov %%ecx, 8(%2) \n\t"
                 "mov %%edx, 12(%2) \n\t"
                 "popa"
                 : : "a"(function), "c"(count), "S"(vec)
                 : "memory", "cc");
#endif /* !__x86_64__ */

    if (eax) {
        *eax = vec[0];
    }
    if (ebx) {
        *ebx = vec[1];
    }
    if (ecx) {
        *ecx = vec[2];
    }
    if (edx) {
        *edx = vec[3];
    }
#endif /* defined(__x86_64__) || defined(__i386__) */
}



int android_get_x86_cpuid_vendor_id_is_vmhost(const char* vendor_id) {
    static const char* const VMHostCPUID[] = {
        "KVMKVMKVM",    // KVM
        "Microsoft Hv", // Microsoft Hyper-V or Windows Virtual PC
        "VMwareVMware", // VMware
        "XenVMMXenVMM", // Xen HVM
    };

    const int VMHostCPUIDCount = sizeof(VMHostCPUID)/sizeof(VMHostCPUID[0]);
    for (int i = 0; i < VMHostCPUIDCount; i++) {
        /* I don't think HAXM supports nesting */
        if (memcmp(vendor_id, VMHostCPUID[i], strlen(VMHostCPUID[i])) == 0)
            return 1;
    }
    return 0;
}

void android_get_x86_cpuid_vendor_id(char* buf, size_t buf_len)
{
    memset(buf, 0, buf_len);
    if (buf_len < 13)
    {
        return;
    }
    android_get_x86_cpuid(0, 0, 0, (uint32_t*)buf, (uint32_t*)(buf+8), (uint32_t*)(buf+4));
}


int android_get_x86_cpuid_vmx_support()
{
    uint32_t cpuid_function1_ecx;
    android_get_x86_cpuid(1, 0, NULL, NULL, &cpuid_function1_ecx, NULL);

    const uint32_t CPUID_1_ECX_VMX = (1<<5); // Intel VMX support
    return (cpuid_function1_ecx & CPUID_1_ECX_VMX) != 0;
}

int android_get_x86_cpuid_svm_support()
{
    uint32_t cpuid_function1_ecx;
    android_get_x86_cpuid(1, 0, NULL, NULL, &cpuid_function1_ecx, NULL);

    const uint32_t CPUID_1_ECX_SVM = (1<<6); // AMD SVM support
    return (cpuid_function1_ecx & CPUID_1_ECX_SVM) != 0;
}

int android_get_x86_cpuid_is_vcpu()
{
    uint32_t cpuid_function1_ecx;
    android_get_x86_cpuid(1, 0, NULL, NULL, &cpuid_function1_ecx, NULL);

    // Hypervisors are supposed to indicate their presence with a bit in the
    // output of running CPUID when eax=1: if the output ECX has bit 31 set
    // (cite: https://lwn.net/Articles/301888/ ).
    const uint32_t CPUID_1_ECX_VCPU = (1<<31); // This is a virtual CPU
    return (cpuid_function1_ecx & CPUID_1_ECX_VCPU) != 0;
}

int android_get_x86_cpuid_nx_support()
{
    if (android_get_x86_cpuid_extended_function_max() < 0x80000001)
        return 0;

    uint32_t cpuid_80000001_edx;
    android_get_x86_cpuid(0x80000001, 0, NULL, NULL, NULL, &cpuid_80000001_edx);

    const uint32_t CPUID_80000001_EDX_NX = (1<<20); // NX support
    return (cpuid_80000001_edx & CPUID_80000001_EDX_NX) != 0;
}


