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

#include "android/utils/x86_cpuid.h"

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
