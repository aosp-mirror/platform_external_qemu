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

#include "android/emulation/goldfish_sync.h"

#include "android/utils/system.h"

static const GoldfishSyncHwFuncs* sGoldfishSyncHwFuncs = NULL;

const GoldfishSyncHwFuncs* goldfish_sync_set_hw_funcs(
        const GoldfishSyncHwFuncs* hw_funcs) {
        const GoldfishSyncHwFuncs* result = sGoldfishSyncHwFuncs;
        sGoldfishSyncHwFuncs = hw_funcs;
        return result;
}

uint64_t goldfish_sync_create_timeline() {
    return sGoldfishSyncHwFuncs->createTimeline();
}

int goldfish_sync_create_fence(uint64_t timeline, uint32_t pt) {
    return sGoldfishSyncHwFuncs->createFence(timeline, pt);
}

void goldfish_sync_timeline_inc(uint64_t timeline, uint32_t howmuch) {
    sGoldfishSyncHwFuncs->incTimeline(timeline, howmuch);
}

void goldfish_sync_destroy_timeline(uint64_t timeline) {
    sGoldfishSyncHwFuncs->destroyTimeline(timeline);
}

#if TEST_GOLDFISH_SYNC

#include <stdio.h>
#include <pthread.h>

void goldfish_sync_run_test() {
    fprintf(stderr, "%s: testing!\n", __FUNCTION__);
}

#endif

