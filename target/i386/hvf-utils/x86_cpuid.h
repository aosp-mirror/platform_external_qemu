/*
 * Copyright (C) 2016 Veertu Inc,
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __CPUID_H__
#define __CPUID_H__

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdarg.h>
#include "qemu-common.h"
#include "x86_flags.h"

struct x86_cpuid {
    const char *name;
    uint32_t level;
    uint32_t vendor1, vendor2, vendor3;
    int family;
    int model;
    int stepping;
    int tsc_khz;
    uint32_t features, ext_features, ext2_features, ext3_features;
    uint32_t kvm_features, svm_features;
    uint32_t xlevel;
    char model_id[48];
    int vendor_override;
    uint32_t flags;
    uint32_t xlevel2;
    uint32_t cpuid_7_0_ebx_features;
};

struct CPUState;

void init_cpuid(struct CPUState* cpu);
void get_cpuid_func(struct CPUState *cpu, int func, int cnt, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

#endif /* __CPUID_H__ */

