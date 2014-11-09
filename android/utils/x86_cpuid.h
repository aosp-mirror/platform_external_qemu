/*
** Copyright (c) 2014, Intel Corporation
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

#ifndef _ANDROID_UTILS_X86_CPUID_H
#define _ANDROID_UTILS_X86_CPUID_H

#include "android/utils/compiler.h"

#include <stdint.h>

ANDROID_BEGIN_HEADER

/* The following list of CPUID features is based on Table 1 in section 'ABI
 * Management' of NDK Programmer's Guide (Android NDK r10c). */
/* Applicable when calling CPUID with EAX=1 */
#define CPUID_EDX_MMX      (1 << 23)
#define CPUID_EDX_SSE      (1 << 25)
#define CPUID_EDX_SSE2     (1 << 26)
#define CPUID_ECX_SSE3     (1 << 0)
#define CPUID_ECX_SSSE3    (1 << 9)
#define CPUID_ECX_SSE41    (1 << 19)
#define CPUID_ECX_SSE42    (1 << 20)
#define CPUID_ECX_POPCNT   (1 << 23)

/*
 * android_get_x86_cpuid: retrieve x86 CPUID for host CPU.
 *
 * Executes the x86 CPUID instruction on the host CPU with the given
 * parameters, and saves the results in the given locations. Does nothing on
 * non-x86 hosts.
 *
 * |function| is the 'CPUID leaf' (the EAX parameter to CPUID), and |count|
 * the 'CPUID sub-leaf' (the ECX parameter to CPUID), givne as input
 *
 * parameters. |eax|,|ebx|,|ecx| and |edx| are optional pointers to variables
 * that will be set on exit to the value of the corresponding register;
 * if one of this parameter is NULL, it is ignored.
 */
void android_get_x86_cpuid(uint32_t function, uint32_t count,
                           uint32_t *eax, uint32_t *ebx,
                           uint32_t *ecx, uint32_t *edx);

ANDROID_END_HEADER

#endif /* _ANDROID_UTILS_X86_CPUID_H */
