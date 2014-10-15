/* Copyright (C) 2014-2015 The Android Open Source Project
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
#ifndef _PROFILE_DEV_H_
#define _PROFILE_DEV_H_

#include "cpu.h"
#include "exec/cpu-defs.h"

void record_mmap(target_ulong vstart, target_ulong vend,
                 target_ulong offset, const char *path, unsigned pid);

void release_mmap(unsigned pid);

void profile_bb_helper(target_ulong pc, uint32_t size);
#endif
