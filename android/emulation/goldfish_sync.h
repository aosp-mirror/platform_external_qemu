/* Copyright (C) 2016 The Android Open Source Project
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

#pragma once

#include "android/utils/compiler.h"

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t goldfish_sync_create_timeline();
void goldfish_sync_create_fence(uint64_t timeline, uint32_t pt);
void goldfish_sync_timeline_inc(uint64_t timeline, uint32_t howmuch);
void goldfish_sync_destroy_timeline(uint64_t timeline);

#ifdef __cplusplus
}
#endif

typedef struct GoldfishSyncHwFuncs {
    uint64_t (*createTimeline)();
    void (*createFence)(uint64_t timeline, uint32_t pt);
    void (*incTimeline)(uint64_t timeline, uint32_t howmuch);
    void (*destroyTimeline)(uint64_t timeline);
} GoldfishSyncHwFuncs;

extern const GoldfishSyncHwFuncs*
goldfish_sync_set_hw_funcs(const GoldfishSyncHwFuncs* hw_funcs);
